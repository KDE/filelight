//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include <kstringhandler.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qptrlist.h>

#include "Config.h"
#include "fileTree.h"
#include "radialMap.h"
#include "sincos.h"
#include "widget.h"



namespace RadialMap
{
   struct Label
   {
      Label( const RadialMap::Segment *s, int l ) : segment( s ), lvl( l ), a( segment->start() + (segment->length() / 2) ) { }

      bool tooClose( const int &aa ) const { return ( a > aa - LABEL_ANGLE_MARGIN && a < aa + LABEL_ANGLE_MARGIN ); }

      const RadialMap::Segment *segment;
      const unsigned int lvl;
      const int a;

      int x1, y1, x2, y2, x3;
      int tx, ty;

      QString qs;
   };

   class LabelList : public QPtrList<Label>
   {
   protected:
      int compareItems( QPtrCollection::Item item1, QPtrCollection::Item item2 )
      {
         //you add 1440 to work round the fact that later you want the circle split vertically
         //and as it is you start at 3 o' clock. It's to do with rightPrevY, stops annoying bug

         int a1 = ((Label*)item1)->a + 1440;
         int a2 = ((Label*)item2)->a + 1440;

         if( a1 == a2 )
            return 0;

         if( a1 > 5760 ) a1 -= 5760;
         if( a2 > 5760 ) a2 -= 5760;

         if( a1 > a2 )
            return 1;

         return -1;
      }
   };
}


void
RadialMap::Widget::paintExplodedLabels( QPainter &paint ) const
{
   //we are a friend of RadialMap::Map

   LabelList list; list.setAutoDelete( true );
   QPtrListIterator<Label> it( list );
   unsigned int startLevel = 0;


   //1. Create list of labels  sorted in the order they will be rendered

   if( m_focus && m_focus->file() != m_tree ) //separate behavior for selected vs unselected segments
   {
      //don't bother with files
      if( m_focus->file() && !m_focus->file()->isDirectory() )
         return;

      //find the range of levels we will be potentially drawing labels for
      //startLevel is the level above whatever m_focus is in
      for( const Directory *p = (const Directory*)m_focus->file(); p != m_tree; ++startLevel )
         p = p->parent();

      //range=2 means 2 levels to draw labels for

      unsigned int a1, a2, minAngle;

      a1 = m_focus->start();
      a2 = m_focus->end();  //boundry angles
      minAngle = int(m_focus->length() * LABEL_MIN_ANGLE_FACTOR);


      #define segment (*it)
      #define ring (m_map.m_signature + i)

      //**** Levels should be on a scale starting with 0
      //**** range is a useless parameter
      //**** keep a topblock var which is the lowestLevel OR startLevel for identation purposes
      for( unsigned int i = startLevel; i <= m_map.m_visibleDepth; ++i )
         for( Iterator<Segment> it = ring->iterator(); it != ring->end(); ++it )
            if( segment->start() >= a1 && segment->end() <= a2 )
               if( segment->length() > minAngle )
                  list.inSort( new Label( segment, i ) );

      #undef ring
      #undef segment

   } else {

      #define ring m_map.m_signature

      for( Iterator<Segment> it = ring->iterator(); it != ring->end(); ++it )
         if( (*it)->length() > 288 )
         list.inSort( new Label( (*it), 0 ) );

      #undef ring

   }

   //2. Check to see if any adjacent labels are too close together
   //   if so, remove the least significant labels

   it.toFirst();
   QPtrListIterator<Label> jt( it );
   ++jt;

   while( jt ) //**** no need to check _it_ as jt will be NULL if _it_ was too
   {
      //this method is fairly efficient

      if( (*it)->tooClose( (*jt)->a ) ) {
         if( (*it)->lvl > (*jt)->lvl ) {
            list.remove( *it );
            it = jt;
         }
         else
            list.remove( *jt );
      }
      else
         ++it;

      jt = it;
      ++jt;
   }

   //used in next two steps
   bool varySizes;
   //**** should perhaps use doubles
   int  *sizes = new int [ m_map.m_visibleDepth + 1 ]; //**** make sizes an array of floats I think instead (or doubles)

   do
   {
      //3. Calculate font sizes

      {
         //determine current range of levels to draw for
         uint range = 0;

         for( it.toFirst(); it != 0; ++it )
         {
            uint lvl = (*it)->lvl;
            if( lvl > range )
               range = lvl;

            //**** better way would just be to assign if nothing is range
         }

         range -= startLevel; //range 0 means 1 level of labels

         varySizes = Config::varyLabelFontSizes && (range != 0);

         if( varySizes )
         {
            //create an array of font sizes for various levels
            //will exceed normal font pitch automatically if necessary, but not minPitch
            //**** this needs to be checked lots

            //**** what if this is negative (min size gtr than default size)
            uint step = (paint.font().pointSize() - Config::minFontPitch) / range;
            if( step == 0 )
               step = 1;

            for( uint x = range + startLevel, y = Config::minFontPitch; x >= startLevel; y += step, --x )
               sizes[x] = y;
         }
      }

      //4. determine label co-ordinates

      int x1, y1, x2, y2, x3, tx, ty; //coords
      double sinra, cosra, ra;  //angles

      int cx = m_map.width()  / 2 + m_offset.x();  //centre relative to canvas
      int cy = m_map.height() / 2 + m_offset.y();

      int spacer, preSpacer = int(m_map.m_ringBreadth * 0.5) + m_map.m_innerRadius;
      int fullStrutLength = ( m_map.width() - m_map.MAP_2MARGIN ) / 2 + LABEL_MAP_SPACER; //full length of a strut from map center

      int prevLeftY  = 0;
      int prevRightY = height();

      bool rightSide;

      QFont font;

      for( it.toFirst(); it != 0; ++it )
      {
         //** bear in mind that text is drawn with QPoint param as BOTTOM left corner of text box
         QString qs = (*it)->segment->file()->name();
         if( varySizes )
            font.setPointSize( sizes[(*it)->lvl] );
         QFontMetrics fm( font );
         int fmh  = fm.height(); //used to ensure label texts don't overlap
         int fmhD4 = fmh / 4;

         fmh += LABEL_TEXT_VMARGIN;

         rightSide = ( (*it)->a < 1440 || (*it)->a > 4320 );

         ra = M_PI/2880 * (*it)->a; //convert to radians
         sincos( ra, &sinra, &cosra );


         spacer = preSpacer + m_map.m_ringBreadth * (*it)->lvl;

         x1 = cx + (int)(cosra * spacer);
         y1 = cy - (int)(sinra * spacer);
         y2 = y1 - (int)(sinra * (fullStrutLength - spacer));

         if( rightSide ) { //righthand side, going upwards
            if( y2 > prevRightY /*- fmh*/ ) //then it is too low, needs to be drawn higher
               y2 = prevRightY /*- fmh*/;
         }
         else //lefthand side, going downwards
            if( y2 < prevLeftY/* + fmh*/ ) //then we're too high, need to be drawn lower
               y2 = prevLeftY /*+ fmh*/;

         x2 = x1 - int(double(y2 - y1) / tan( ra ));
         ty = y2 + fmhD4;


         if( rightSide ) {
            if( x2 > width() || ty < fmh || x2 < x1 ) {
               //skip this strut
               //**** don't duplicate this code
               list.remove( *it ); //will delete the label and set it to list.current() which _should_ be the next ptr
               break;
            }

            prevRightY = ty - fmh - fmhD4; //must be after above's "continue"

            qs = KStringHandler::cPixelSqueeze( qs, fm, width() - x2 );

            x3 = width() - fm.width( qs )
                  - LABEL_HMARGIN //outer margin
                  - LABEL_TEXT_HMARGIN //margin between strut and text
                  //- ((*it)->lvl - startLevel) * LABEL_HMARGIN; //indentation
                  ;
            if( x3 < x2 ) x3 = x2;
            tx = x3 + LABEL_TEXT_HMARGIN;

         } else {

            if( x2 < 0 || ty > height() || x2 > x1 )
            {
               //skip this strut
               list.remove( *it ); //will delete the label and set it to list.current() which _should_ be the next ptr
               break;
            }

            prevLeftY = ty + fmh - fmhD4;

            qs = KStringHandler::cPixelSqueeze( qs, fm, x2 );

            //**** needs a little tweaking:

            tx = fm.width( qs ) + LABEL_HMARGIN/* + ((*it)->lvl - startLevel) * LABEL_HMARGIN*/;
            if( tx > x2 ) { //text is too long
               tx = LABEL_HMARGIN + x2 - tx; //some text will be lost from sight
               x3 = x2; //no text margin (right side of text here)
            } else {
               x3 = tx + LABEL_TEXT_HMARGIN;
               tx = LABEL_HMARGIN /*+ ((*it)->lvl - startLevel) * LABEL_HMARGIN*/;
            }
         }

         (*it)->x1 = x1;
         (*it)->y1 = y1;
         (*it)->x2 = x2;
         (*it)->y2 = y2;
         (*it)->x3 = x3;
         (*it)->tx = tx;
         (*it)->ty = ty;
         (*it)->qs = qs;
      }

      //if an element is deleted at this stage, we need to do this whole
      //iteration again, thus the following loop
      //**** in rare case that deleted label was last label in top level
      //     and last in labelList too, this will not work as expected (not critical)

   } while( it != 0 );


   //5. Render labels

   paint.setPen( QPen( Qt::black, 1 ) );

   for( it.toFirst(); it != 0; ++it )
   {
      if( varySizes ) {
         //**** how much overhead in making new QFont each time?
         //     (implicate sharing remember)
         QFont font = paint.font();
         font.setPointSize( sizes[(*it)->lvl] );
         paint.setFont( font );
      }

      paint.drawEllipse( (*it)->x1 - 3, (*it)->y1 - 3, 7, 7 ); //**** CPU intensive! better to use a pixmap
      paint.drawLine(  (*it)->x1,  (*it)->y1, (*it)->x2, (*it)->y2 );
      paint.drawLine( (*it)->x2, (*it)->y2, (*it)->x3, (*it)->y2);
      paint.drawText( (*it)->tx, (*it)->ty, (*it)->qs );
   }

   delete [] sizes;
}
