/***************************************************************************
                          filelightcanvas.cpp  -  description
                             -------------------
    begin                : Sun May 25 2003
    copyright            : (C) 2003 by Max Howell
    email                : mh9193@bris.ac.uk

    contents             : handles paint and mouseMove Events

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <math.h>        //segmentAt()

#include <qpainter.h>
#include <qtimer.h>      //resizeEvent()
#include <qstatusbar.h>

#include <kaction.h>
#include <kiconeffect.h> //mousePressEvent() [visual feedback]
#include <kcursor.h>     //mouseMoveEvent()  [hand cursor]
#include <kpopupmenu.h>  //right click popupmenu

#include "kradialmap.h"
#include "../filetree.h"
#include "../settings.h"



extern Settings Gsettings;
extern unsigned int MAX_RING_DEPTH;


void KRadialMap::resizeEvent( QResizeEvent * )
{
  if( m_map.resize( rect() ) )
    m_timer->start( 200, true ); //will cause signature to rebuild for new size

  //always do these as they need to be initialised on creation
  m_offset.rx() = (width() - m_map.width()) / 2;
  m_offset.ry() = (height() - m_map.height()) / 2;
}


void KRadialMap::paintEvent( QPaintEvent * )
{
  QPainter paint( this );  
  //bltBit for some Qt setups will bitBlt _after_ the labels are painted. Which buggers things up!
  //shame as bitBlt is faster, possibly Qt bug? Should report the bug?
//  bitBlt( this, m_xOffset, m_yOffset, m_map );
  paint.drawPixmap( m_offset, m_map );

  //** going on principle it's efficient to exceed lengths on right and bottom
  //vertical stripes
  if( m_map.width() < width() ) {
    paint.eraseRect( 0, 0, m_offset.x(), height() );
    paint.eraseRect( m_map.width() + m_offset.x(), 0, m_offset.x(), height() );
  }
  //horizontal stripes
  if( m_map.height() < height() ) {
    paint.eraseRect( 0, 0, width(), m_offset.y() );
    paint.eraseRect( 0, m_map.height() + m_offset.y(), width(), m_offset.y() );
  }

  //exploded labels
  if( !m_map.isNull() )
    paintExplodedLabels( paint );
}


const KRadialMap::Segment *KRadialMap::segmentAt( QPoint &e ) const
{
  //determine which segment QPoint e is above
  
  e -= m_offset;

  if( e.x() <= m_map.width() && e.y() <= m_map.height() )
  {
    //transform to cartesian coords
    e.rx() -= m_map.width() / 2; //should be an int
    e.ry()  = m_map.height() / 2 - e.y();

    float length = hypot( e.x(), e.y() );

    if( length >= m_map.m_innerRadius ) //not hovering over inner circle
    {
      unsigned int depth  = ((int)length - m_map.m_innerRadius) / m_map.m_ringBreadth;

      if( depth <= m_map.m_visibleDepth ) //**** do earlier since you can //** check not outside of range
      {

        //vector calculation, reduces to simple trigonometry
        //cos angle = (aibi + ajbj) / albl
        //ai = x, bi=1, aj=y, bj=0
        //cos angle = x / (length)

        unsigned int a  = (unsigned int)(acos( (float)e.x() / length ) * 916.736);  //916.7324722 = #radians in circle * 16

        //acos only understands 0-180 degrees
        if( e.y() < 0 )
          a = 5760 - a;

        #define ring (m_map.m_signature + depth)

        for( ConstIterator<Segment> it = ring->constIterator(); it != ring->end(); ++it )
          if( (*it)->intersects( a ) )
            return *it;

        #undef ring
      }
    } else return m_map.m_rootSegment; //hovering over inner circle
  }

  return NULL;
}


void KRadialMap::mouseMoveEvent( QMouseEvent *e )
{
  //set m_focus to what we hover over

  const Segment *oldFocus = m_focus;
  QPoint p = e->pos();

  m_focus = segmentAt( p ); //p is passed by reference (non-const)

  if( m_focus != NULL && m_focus->file() != m_tree ) {
    if( m_focus != oldFocus ) //if not same as last time
    {
      setCursor( KCursor::handCursor() );
      //FIXME much redundancy in calculating focus path, you do this SO many times!
      m_tip.updateTip( m_focus->file(), m_tree );
      m_status->message( fullPath( m_focus->file() ) );
      repaint( false ); //repaint required to update labels now before transparency is generated
    }

    m_tip.moveto( mapToGlobal( e->pos() ), *this, ( p.y() < 0 ) ); //updates tooltip psuedo-tranparent background
  }
  else if( oldFocus != NULL && oldFocus->file() != m_tree )
  {
    unsetCursor();
    m_tip.hide();
    m_status->clear();
    update();
  }
}


void KRadialMap::mousePressEvent( QMouseEvent * e )
{
  //m_tip hidden already by event filter
  //m_focus is set correctly (I've been strict, I assure you it is correct!)
  if( m_focus == NULL )
    return;
  if( m_focus->isFake() ) //fake segments are the ones that represent more than one file
    return;

  bool b = m_focus->file()->isDir();

  if( e->button() == Qt::RightButton )
  {
    KPopupMenu popup;

    popup.insertTitle( fullPath( m_focus->file(), m_tree ) );

    if( b )
    {
      m_actKonqi->plug( &popup );
      m_actKonsole->plug( &popup );
      if( m_focus->file() != m_tree )
      {
        popup.insertSeparator();
        m_actCenter->plug( &popup );
      }
    }
    else m_actRun->plug( &popup );

    //asynchronous, i.e. rest of Filelight halts while popup is displayed
    if( popup.exec( QWidget::mapToGlobal( e->pos() ), 1 ) < 0 )
      slotPostMouseEvent(); //ensure m_focus is set for new position if popup cancelled
  }
  else
  {
    QRect rect( e->x() - 15, e->y() - 15, 25, 25 );

    if( !b || e->button() == Qt::MidButton )
    {
      KIconEffect::visualActivate( this, rect );
      slotRun();
    }
    else if( m_focus->file() != m_tree )
    {
      KIconEffect::visualActivate( this, rect );
      m_actCenter->activate();
    }
  }
}
