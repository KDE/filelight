//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include <kcursor.h>        //ctor
#include <klocale.h>
#include <kurl.h>
#include <qapplication.h>   //sendEvent
#include <qbitmap.h>        //ctor - finding cursor size
#include <qcursor.h>        //slotPostMouseEvent()
#include <qtimer.h>         //member

#include "Config.h"
#include "debug.h"
#include "fileTree.h"
#include "radialMap.h" //constants
#include "widget.h"



RadialMap::Widget::Widget( QWidget *parent, const char *name )
   : QWidget( parent, name, Qt::WNoAutoErase )
   , m_tree( 0 )
   , m_focus( 0 )
   , m_tip( KCursor::handCursor().bitmap()->height() ) //needs to know cursor height
   , m_rootSegment( 0 ) //TODO we don't delete it, *shrug*
{
   setBackgroundColor( Qt::white );

   connect( this, SIGNAL(created( const Directory* )), SLOT(sendFakeMouseEvent()) );
   connect( this, SIGNAL(created( const Directory* )), SLOT(update()) );
   connect( &m_timer, SIGNAL(timeout()), SLOT(resizeTimeout()) );
}

QString
RadialMap::Widget::path() const
{
   return m_tree->fullPath();
}

KURL
RadialMap::Widget::url( File const * const file ) const
{
   return KURL::fromPathOrURL( file ? file->fullPath() : m_tree->fullPath() );
}

void
RadialMap::Widget::invalidate( const bool b )
{
   if( isValid() )
   {
      //**** have to check that only way to invalidate is this function frankly
      //**** otherwise you may get bugs..

      //disable mouse tracking
      setMouseTracking( false );

      //ensure this class won't think we have a map still
      m_tree  = 0;
      m_focus = 0;

      delete m_rootSegment;
      m_rootSegment = 0;

      //FIXME move this disablement thing no?
      //      it is confusing in other areas, like the whole createFromCache() thing
      m_map.invalidate( b ); //b signifies whether the pixmap is made to look disabled or not
      if( b )
         update();

      //tell rest of Filelight
      emit invalidated( url() );
   }
}

void
RadialMap::Widget::create( const Directory *tree )
{
   //it is not the responsibility of create() to invalidate first
   //skip invalidation at your own risk

   //FIXME make it the responsibility of create to invalidate first

   if( tree )
   {
      //generate the filemap image
      m_map.make( tree );

      //this is the inner circle in the center
      m_rootSegment = new Segment( tree, 0, 16*360 );

      setMouseTracking( true );
   }

   m_tree = tree;

   //tell rest of Filelight
   emit created( tree );
}

void
RadialMap::Widget::createFromCache( const Directory *tree )
{
    //no scan was necessary, use cached tree, however we MUST still emit invalidate
    invalidate( false );
    create( tree );
}

void
RadialMap::Widget::sendFakeMouseEvent() //slot
{
   QMouseEvent me( QEvent::MouseMove, mapFromGlobal( QCursor::pos() ), Qt::NoButton, Qt::NoButton );
   QApplication::sendEvent( this, &me );
}

void
RadialMap::Widget::resizeTimeout() //slot
{
   // the segments are about to erased!
   // this was a horrid bug, and proves the OO programming should be obeyed always!
   m_focus = 0;
   if( m_tree )
      m_map.make( m_tree, true );
   update();
}

void
RadialMap::Widget::refresh( int filth )
{
   //TODO consider a more direct connection

   if( !m_map.isNull() )
   {
      switch( filth )
      {
      case 1:
         m_map.make( m_tree, true ); //true means refresh only
         break;

      case 2:
         m_map.aaPaint();
         break;

      case 3:
         m_map.colorise(); //FALL THROUGH!
      case 4:
         m_map.paint();

      default:
         break;
      }

      update();
   }
}

void
RadialMap::Widget::zoomIn() //slot
{
   if( m_map.m_visibleDepth > MIN_RING_DEPTH )
   {
      --m_map.m_visibleDepth;
      m_map.make( m_tree );
      Config::defaultRingDepth = m_map.m_visibleDepth;
      update();
   }
}

void
RadialMap::Widget::zoomOut() //slot
{
   ++m_map.m_visibleDepth;
   m_map.make( m_tree );
   if( m_map.m_visibleDepth > Config::defaultRingDepth )
      Config::defaultRingDepth = m_map.m_visibleDepth;
   update();
}


RadialMap::Segment::~Segment()
{
   if( isFake() )
      delete m_file; //created by us in Builder::build()
}

#include "widget.moc"
