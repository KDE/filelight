/***************************************************************************
                          filemap.cpp  -  description
                             -------------------
    begin                : Sun Jul 13 2003
    copyright            : (C) 2003 by Max Howell
    email                : max.howell@methylblue.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <math.h>   //used while painting the map

#include <qapplication.h>  //make()
#include <qpainter.h>
#include <qimage.h>        //make() & paint()
#include <qfont.h>
#include <qfontmetrics.h>

#include <kconfig.h>       //loading kde colours
#include <kpixmap.h>       //derived from
#include <kimageeffect.h>  //desaturate()
#include <kdebug.h>
#include <kcursor.h>       //make()

#include "define.h"
#include "builder.h"
#include "filemap.h"
#include "settings.h"
#include "filetree.h"



#define COLOR_GREY QColor( 0, 0, 140, QColor::Hsv )

extern Settings Gsettings;
unsigned int MAX_RING_DEPTH = DEFAULT_MAX_RING_DEPTH;


FileMap::FileMap() :
    m_signature( NULL ),
    m_ringBreadth( MIN_RING_BREADTH ),
    m_innerRadius( 0 ),
    m_visibleDepth( 0 ),
    m_glob( "" )
{

  //optimise for speed
  setOptimization( QPixmap::BestOptim );

  KConfig *config = KGlobal::config();

  config->setGroup( "WM" );
  kdeColour[0] = config->readColorEntry( "activeBackground", 0 ); //**** need default colours!
  config->setGroup( "General" );
  kdeColour[1] = config->readColorEntry( "selectBackground", 0 );

/*  for( int h,s,v,i=0; i < 2; ++i )
  {
    kdeColour[i].hsv( &h, &s, &v );
    kdDebug() << s << endl;
    if( s < 180 )
      kdeColour[i].setHsv( h, 180, v );
  }*/
  
  deltaRed   = (double)(kdeColour[0].red()   - kdeColour[1].red())   / 2880; //2880 for semicircle
  deltaGreen = (double)(kdeColour[0].green() - kdeColour[1].green()) / 2880;
  deltaBlue  = (double)(kdeColour[0].blue()  - kdeColour[1].blue())  / 2880;

  QFont font;
  if( Gsettings.varyLabelFontSizes )
    font.setPointSize( Gsettings.minFontPitch + MAX_RING_DEPTH );
  int fmh   = QFontMetrics( font ).height();
  int fmhD4 = fmh / 4;
  MAP_2MARGIN = 2 * ( fmh - (fmhD4 - LABEL_MAP_SPACER) ); //margin is dependent on fitting in labels at top and bottom


}


FileMap::~FileMap()
{
  delete [] m_signature;
}


void FileMap::invalidate( const bool &b )
{
  delete [] m_signature;
  m_signature = NULL;

  if( b )
  {
    QImage img = this->convertToImage();

    KImageEffect::desaturate( img, 0.7 );
    KImageEffect::toGray( img, true );

    this->convertFromImage( img );
  }
}


void FileMap::make( const Directory *tree, bool refresh )
{
  //**** determineText seems pointless optimisation
  //   but is it good to keep the text consistent?
  //   even if it makes it a lie?

  //slow operation so set the wait cursor
  QApplication::setOverrideCursor( KCursor::waitCursor() );
  
  {
    //build a signature of visible components    
    delete [] m_signature;
    Builder builder( this, tree, refresh );
  }

  //colour the segments
  colorise();

  //determine centerText
  if( !refresh )
  {
    int i;
    
    for( i = 2; i > 0; --i )
      if( tree->size() > UNIT_DENOMINATOR[i] )
        break;

    m_centerText = makeHumanReadable( tree->size(), (UnitPrefix)i );
  }

  //paint the pixmap
  aaPaint();

  QApplication::restoreOverrideCursor();  
}


void FileMap::setRingBreadth()
{
  m_ringBreadth = (height() - MAP_2MARGIN) / (2 * m_visibleDepth + 4);

  if( m_ringBreadth < MIN_RING_BREADTH )
    m_ringBreadth = MIN_RING_BREADTH;
  else if( m_ringBreadth > MAX_RING_BREADTH )
    m_ringBreadth = MAX_RING_BREADTH;
}


bool FileMap::resize( const QRect &rect )
{
  //there's a MAP_2MARGIN border

  #define mw width()
  #define mh height()
  #define cw rect.width()
  #define ch rect.height()

  if( cw < mw || ch < mh || (cw > mw && ch > mh) )
  {
    int size = (( cw < ch ) ? cw : ch) - MAP_2MARGIN;
//this also causes uneven sizes to always resize when resizing but map is small in that dimension
//    size -= size % 2; //even sizes mean less staggered non-antialiased resizing

    #undef mw
    #undef mh
    #undef cw
    #undef ch

    {
      int minSize = MIN_RING_BREADTH * 2 * (m_visibleDepth + 2);
      if( size < minSize )
        size = minSize;
      
      int m = MAP_2MARGIN / 2;      
      //this QRect is used by paint()
      m_rect.setRect( m, m, size, size );
    }
      
    //resize the pixmap
    size += MAP_2MARGIN;
    KPixmap::resize( size, size );

    if( m_signature != NULL )
    {
      setRingBreadth();
      paint();
    }
    else
      fill(); //**** don't like having to do this..

    return true;
  }

  return false;
}


void FileMap::colorise()
{
  QColor cp, cb;
  double darkness = 1;
  double contrast = (double)Gsettings.contrast / (double)100;
  int h, s1, s2, v1, v2;
  
  for( unsigned int d = 0; d <= m_visibleDepth; ++d, darkness += 0.04 )
  {
    #define ring (m_signature + d)

    for( Iterator<Segment> it = ring->iterator(); it != ring->end(); ++it )
    {
      
      
      /*
      switch( Gsettings.scheme )
      {
      case scheme_kde:
        {

      //gradient will work by figuring out rgb delta values for 360 degrees
      //then each component is angle*delta

        int h, s, v, a = (*it)->start();

        if( a > 2880 ) a = 2880 - (a - 2880);

        h = (int)(deltaRed * a) + kdeColour[1].red();
        s = (int)(deltaGreen * a) + kdeColour[1].green();
        v = (int)(deltaBlue * a) + kdeColour[1].blue();

        cb.setRgb( h, s, v );
        cb.hsv( &h, &s, &v );

        v = int((float)v / darkness);

        cb.setHsv( h, s, v );
        cp.setHsv( h, 255, v - CONTRAST );

        }
        break;

      case scheme_highContrast:

        cp.setHsv( 0, 0, 0 ); //values of h, s and v are irrelevant
        cb.setHsv( 0, 0, 155 + CONTRAST );

        break;

      default:
        {
          int v = int(255 / darkness);
          int h = int((*it)->start() / 16);
          
          if( (*it)->file()->isDir() || (*it)->isFake() )
          {
            cb.setHsv( h, 160, v );
            cp.setHsv( h, 255, v - CONTRAST );
          }
          else
          {
            cb.setHsv( h, 20, 225 );
            cp.setHsv( h, 20, 225 - CONTRAST );//v - CONTRAST );
          }
        }
      }*/

      switch( Gsettings.scheme )
      {
      case scheme_kde:
      {
        //gradient will work by figuring out rgb delta values for 360 degrees
        //then each component is angle*delta

        int a = (*it)->start();

        if( a > 2880 ) a = 2880 - (a - 2880);

        h  = (int)(deltaRed   * a) + kdeColour[1].red();
        s1 = (int)(deltaGreen * a) + kdeColour[1].green();
        v1 = (int)(deltaBlue  * a) + kdeColour[1].blue();

        cb.setRgb( h, s1, v1 );
        cb.hsv( &h, &s1, &v1 );

        kdDebug() << s1 << endl;
        
        break;
      }

      case scheme_highContrast:

        cp.setHsv( 0, 0, 0 ); //values of h, s and v are irrelevant
        cb.setHsv( 180, 0, int(255.0 * contrast) );
        (*it)->setPalette( cp, cb );
        continue;
        
      default:
        h  = int((*it)->start() / 16);
        s1 = 160;
        v1 = (int)(255.0 / darkness); //****doing this more often than once seems daft!
      }

      v2 = v1 - static_cast<int>(contrast * v1);
      s2 = s1 + static_cast<int>(contrast * (255 - s1));

      if( s1 < 80 ) s1 = 80; //can fall too low and makes contrast between the files hard to discern
      
      if( (*it)->isFake() ) //multi-file
      {
        cb.setHsv( h, s2, (v2 < 90) ? 90 : v2 ); //too dark if < 100
        cp.setHsv( h, 17, v1 );
      }
      else if( !(*it)->file()->isDir() ) //file
      {
        cb.setHsv( h, 17, v1 );
        cp.setHsv( h, 17, v2 );
      }
      else //directory
      {
        cb.setHsv( h, s1, v1 ); //v was 225
        cp.setHsv( h, s2, v2 ); //v was 225 - delta
      }            

      (*it)->setPalette( cp, cb );

      //**** may be better to store KDE colours as H and S and vary V as others
      //**** perhaps make saturation difference for s2 dependent on contrast too
      //**** fake segments don't work with highContrast
      //**** may work better with cp = cb rather than Qt::white
      //**** you have to ensure the grey of files is sufficient, currently it works only with rainbow (perhaps use contrast there too)
      //**** change v1,v2 to vp, vb etc.
      //**** using percentages is not strictly correct as the eye doesn't work like that
      //**** darkness factor is not done for kde_colour scheme, and also value for files is incorrect really for files in this scheme as it is not set like rainbow one is
    }

    #undef ring
  }
}


/**********************************************
  PAINT the filemap image
 **********************************************/

//** inlining this gave linking errors
void FileMap::aaPaint()
{
  //paint() is called during continuous processes
  //aaPaint() is not and is slower so set overidecursor (make sets it too)
  QApplication::setOverrideCursor( KCursor::waitCursor() );
  paint( Gsettings.aaFactor );
  QApplication::restoreOverrideCursor();

  //**** maybe paint should accept a bool for wait cursor / in background cursor
}

void FileMap::paint( unsigned int scaleFactor )
{
  QPainter paint;
  QRect rect = m_rect;
  int step = m_ringBreadth;
  int excess = -1;

  //scale the pixmap, or do intelligent distribution of excess to prevent nasty resizing
  if( scaleFactor > 1 )
  {
    int x1, y1, x2, y2;
    rect.coords( &x1, &y1, &x2, &y2 );
    x1 *= scaleFactor;
    y1 *= scaleFactor;
    x2 *= scaleFactor;
    y2 *= scaleFactor;
    rect.setCoords( x1, y1, x2, y2 );

    step *= scaleFactor;
    KPixmap::resize( this->size() * (int)scaleFactor );
  }
  else if( m_ringBreadth != MAX_RING_BREADTH && m_ringBreadth != MIN_RING_BREADTH ) {
    excess = rect.width() % m_ringBreadth;
    ++step;
  }

  //**** best option you can think of is to make the circles slightly less perfect,
  //  ** i.e. slightly eliptic when resizing inbetween


  paint.begin( this );

  fill(); //erase background

  
  #define ring (m_signature + x)

  for( int x = m_visibleDepth; x >= 0; --x )
  {
    int width = rect.width() / 2;
    //clever geometric trick to find largest angle that will give biggest arrow head
    int a_max = acos( (double)width / double(width + 5 * scaleFactor) ) * (180*16 / M_PI);

    for( ConstIterator<Segment> it = ring->constIterator(); it != ring->end(); ++it )
    {
      //draw the pie segments, most of this code is concerned with drawing the little
      //arrows on the ends of segments when they have hidden files

      paint.setPen( (*it)->pen() );

      if( (*it)->hasHiddenChildren() )
      {
        //draw arrow head to indicate undisplayed files/directories
        QPointArray pts( 3 );
        QPoint pos, cpos = rect.center();
        int a[3] = { (*it)->start(), (*it)->length(), 0 };

        a[2] = a[0] + (a[1] / 2); //assign to halfway between
        if( a[1] > a_max )
        {
          a[1] = a_max;
          a[0] = a[2] - a_max / 2;
        }

        a[1] += a[0];

        for( int i = 0, radius = width; i < 3; ++i )
        {
          double ra = M_PI/(180*16) * a[i], sinra, cosra;

          if( i == 2 )
            radius += 5 * scaleFactor;
          sincos( ra, &sinra, &cosra );
          pos.rx() = cpos.x() + static_cast<int>(cosra * radius);
          pos.ry() = cpos.y() - static_cast<int>(sinra * radius);
          pts.setPoint( i, pos );
        }

        paint.setBrush( (*it)->pen() );
        paint.drawPolygon( pts );
      }      

      paint.setBrush( (*it)->brush() );
      paint.drawPie( rect, (*it)->start(), (*it)->length() );

      if( (*it)->hasHiddenChildren() )
      {
        //**** code is bloated!
        paint.save();
        QPen pen = paint.pen();
        int width = 2 * scaleFactor;
        pen.setWidth( width );
        paint.setPen( pen );
        QRect rect2 = rect;
        width /= 2;
        rect2.addCoords( width, width, -width, -width );
        paint.drawArc( rect2, (*it)->start(), (*it)->length() );
        paint.restore();
      }
    }

    if( excess >= 0 ) { //excess allows us to resize more smoothly (still crud tho)
      if( excess < 2 ) //only decrease rect by more if even number of excesses left
        --step;
      excess -= 2;
    }

    rect.addCoords( step, step, -step, -step );
  }

//  if( excess > 0 ) rect.addCoords( excess, excess, 0, 0 ); //ugly

  paint.setPen( COLOR_GREY );
  paint.setBrush( Qt::white );
  paint.drawEllipse( rect );

  if( scaleFactor > 1 )
  {
    //have to end in order to smoothscale()
    paint.end();

    int x1, y1, x2, y2;
    rect.coords( &x1, &y1, &x2, &y2 );
    x1 /= scaleFactor;
    y1 /= scaleFactor;
    x2 /= scaleFactor;
    y2 /= scaleFactor;
    rect.setCoords( x1, y1, x2, y2 );


    QImage img = this->convertToImage();
    img = img.smoothScale( this->size() / (int)scaleFactor );
    this->convertFromImage( img );


    paint.begin( this );

    paint.setPen( COLOR_GREY );
    paint.setBrush( Qt::white );
  }

  paint.drawText( rect, Qt::AlignCenter, m_centerText );

  m_innerRadius = rect.width() / 2; //rect.width should be multiple of 2

  paint.end();

  #undef ring
}
