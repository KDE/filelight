/**************************************************************************
                          segmenttip.cpp  -  description
                             -------------------
    begin                : Sat Jul 12 2003
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

#include <qtimer.h>
#include <qpainter.h>
#include <qtooltip.h> //for its palette

#include <kapplication.h>     //installing eventFilters
#include <kglobal.h>
#include <klocale.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>

#include <stdlib.h>     //abs

#include "kradialmap.h"
#include "../filetree.h"



KRadialMap::SegmentTip::SegmentTip( unsigned int h ) :
    QWidget( 0, 0, WStyle_Customize | WStyle_NoBorder | WStyle_Tool | WStyle_StaysOnTop | WX11BypassWM ),
    m_cursorHeight( -h ),
    m_timer( new QTimer( this, "tooltip_timer" ) )
{
  setWFlags( Qt::WResizeNoErase );
  setBackgroundMode( Qt::NoBackground );

  m_pixmap.setOptimization( QPixmap::BestOptim );

  connect( m_timer, SIGNAL( timeout() ), this, SLOT( hide() ) );
}

KRadialMap::SegmentTip::~SegmentTip()
{}

 
void KRadialMap::SegmentTip::moveto( QPoint p, const QWidget &canvas, bool placeAbove )
{
  //**** this function is very slow and seems to be visibly influenced by operations like mapFromGlobal() (who knows why!)
  //  ** so any improvements are much desired

  m_timer->start( 8000, true ); //singleshot


  
  p.rx() -= rect().center().x();
  p.ry() -= (placeAbove ? 8 + height() : m_cursorHeight - 8);

//**** use when using KDE 3.2
//  static QRect KGlobalSettings::desktopGeometry(QWidget *w); 
  const QRect screen = QApplication::desktop()->screenGeometry(-1);

  int x  = p.x();
  int y  = p.y();
  int x2 = x + width();
  int y2 = y + height(); //how's it ever gunna get below screen height?! (well you never know I spose)
  int sw = screen.width();
  int sh = screen.height();
  
  if( x  < 0  ) p.setX( 0 );
  if( y  < 0  ) p.setY( 0 );
  if( x2 > sw ) p.rx() -= x2 - sw;
  if( y2 > sh ) p.ry() -= y2 - sh;


  //I'm using this QPoint to determine where to offset the bitBlt in m_pixmap
  QPoint gumpy = canvas.mapToGlobal( QPoint( 0, 0 ) ) - p;
  if( gumpy.x() < 0 ) gumpy.setX( 0 );
  if( gumpy.y() < 0 ) gumpy.setY( 0 );  

  //**** slow to get color this way? maybe a member instead (but then updates won't reflect.. )
  QColor tipBgColor = QToolTip::palette().color( QPalette::Active, QColorGroup::Background );
  QRect alphaMaskRect( canvas.mapFromGlobal( p ), size() );
  QRect intersection( alphaMaskRect.intersect( canvas.rect() ) );

  m_pixmap.resize( size() );
  bitBlt( &m_pixmap, gumpy, &canvas, intersection, Qt::CopyROP );

//  kdDebug() << intersection.x() << ", " << intersection.y() << " | "<< gumpy.x() << ", " << gumpy.y() << endl;

  QPainter paint;
    paint.begin( &m_pixmap );
    paint.setPen( Qt::black );     //**** redundant? defaults?
    paint.setBrush( Qt::NoBrush ); //**** redundant? defaults?
    paint.drawRect( rect() );
  paint.end();

  m_pixmap = KPixmapEffect::fade( m_pixmap, 0.6, tipBgColor );

  paint.begin( &m_pixmap );
  paint.drawText( rect(), AlignCenter, m_text );
  paint.end();
      
  move( p );
  if( !isVisible() ) show(); //must be called before update and doesn't call it itself apparently
  update();
}


void KRadialMap::SegmentTip::update()
{
  // on other hand you could do transformation of whole pixmap on initial draw and store it here
  // **** good plan! (bit mem hogging, but will be much quicker to do..)
  // **** aaah, will  only work if you can bitBlt current label pattern on top too (with quick transformation to slightly grayer appearance (pixmap can do no?)
  // **** think about this at some point, it would be very cool
  
  //was bitBlt, but debian users couldn't see transparency! what is it with bitBlting?

  //**** blt might work now that there is no painting over the blt..
  QPainter p( this );
  p.drawPixmap( 0, 0, m_pixmap );
}



void KRadialMap::SegmentTip::updateTip( const File* const file, const Directory* const root )
{
  QString qs1, qs2;

  qs1 = fullPath( file, root );
  
  #define MARGIN 3

  KLocale *loc      = KGlobal::locale();
  QFontMetrics fm   = QWidget::fontMetrics();
  int pc   = 100 * file->size() / root->size();
  int y    = fm.height() * 2 + 2 * MARGIN;
  int maxw = 0, w;  

  qs2 = makeHumanReadable( file->size() );//, key ); //returns a human readable string for a filesize
  
  if( pc > 0 )
    qs2 += QString( " (%1%)" ).arg( loc->formatNumber( pc, 0 ) );

  m_text = qs1 + "\n" + qs2;
      
  if( const Directory *dir = dynamic_cast<const Directory *>(file) )
  {
    double fcnt = dir->fileCount();
    pc          = int(100 * fcnt / (double)root->fileCount());
    QString qs3 = i18n( "Files: %1" ).arg( loc->formatNumber( fcnt, 0 ) );
    if( pc > 0 )
      qs3 += " (" + loc->formatNumber( pc, 0 ) + "%)";
    maxw = fm.width( qs3 );
    y   += fm.height();
    m_text += "\n" + qs3;
  }


  w = fm.width( qs1 ); if( w > maxw ) maxw = w;
  w = fm.width( qs2 ); if( w > maxw ) maxw = w;

  resize( maxw + 2 * MARGIN, y );

            
  #undef MARGIN
}


void KRadialMap::SegmentTip::showEvent( QShowEvent * )
{
  kapp->installEventFilter( this );
}

void KRadialMap::SegmentTip::hideEvent( QHideEvent * )
{
  kapp->removeEventFilter( this );
}


bool KRadialMap::SegmentTip::eventFilter( QObject *, QEvent *e )
{
    switch ( e->type() )
    {
        case QEvent::Leave:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Wheel:
            hide();
        default: break;
    }

    return false; //allow this event to passed to target
}
