/***************************************************************************
                          filelightcanvas.cpp  -  description
                             -------------------
    begin                : Sun May 25 2003
    copyright            : (C) 2003 by Max Howell
    email                : mh9193@bris.ac.uk

    contents             : handles paint and mouseMove Events

    terms    - m_signature : an array of lists, one for each subdir depth,
                           contains pointer to a file and info on how to
                           represent the file as ring segments

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   The method and concept of using concentric pie segment rings to       *
 *   represent the layout of files in a filesystem is copyright Steffen    *
 *   Gerlach, http://www.steffengerlach.de/freeware/ permission was        *
 *   granted to use the methodology with this program.                     *
 *                                                                         *
 ***************************************************************************/

#include <qapplication.h>   //sendEvent
#include <qtimer.h>         //member
#include <qcursor.h>        //slotPostMouseEvent()
#include <qbitmap.h>        //ctor - finding cursor size

#include <kmainwindow.h>    //member *
#include <kaction.h>        //member
#include <krun.h>           //slotSpawnKonqi(), pupup slots
#include <kmessagebox.h>
#include <kpopupmenu.h>     //ctor
#include <kcursor.h>        //ctor
#include <klocale.h>

#include "kradialmap.h"
#include "../settings.h"
#include "../filetree.h"


extern Settings Gsettings;

KRadialMap::KRadialMap( QWidget *parent, const char *name )
  : QWidget( parent, name )
  , m_tree( 0 )
  , m_focus( 0 )
  , m_timer( new QTimer( this ) )
  , m_tip( KCursor::handCursor().bitmap()->height() ) //needs to know cursor height
{
  setWFlags( Qt::WRepaintNoErase ); //prevent flicker
  setWFlags( Qt::WResizeNoErase );  //prevent flicker
  setBackgroundColor( Qt::white );

  m_actionJug  = new KActionCollection( this );
  m_actCenter  = new KAction( i18n( "&Center Map Here" ), "viewmag", 0, this, SLOT( slotCenterHere() ), m_actionJug, "center_here" );
  m_actKonqi   = new KAction( i18n( "Open &Konqueror Here" ), "konqueror", 0, this, SLOT( slotKonqiHere() ), m_actionJug, "konqueror_here" );
  m_actKonsole = new KAction( i18n( "Open Konsole &Here" ), "konsole", 0, this, SLOT( slotKonsoleHere() ), m_actionJug, "konsole_here" );
  m_actRun     = new KAction( i18n( "&Open" ), "fileopen", 0, this, SLOT( slotRun() ), m_actionJug, "run" );

  connect( this, SIGNAL( created( const Directory * ) ), SLOT( slotPostMouseEvent() ) );
  connect( this, SIGNAL( created( const Directory * ) ), SLOT( update() ) );
  connect( m_timer, SIGNAL( timeout() ), this, SLOT( slotResizeTimeout() ) );
}


KRadialMap::~KRadialMap()
{}


void KRadialMap::invalidate( const bool &b )
{
  if( isValid() )
  {
    //**** have to check that only way to invalidate is this function frankly
    //**** otherwise you may get bugs..

    const KURL url( m_path );

    //disable mouse tracking
    setMouseTracking( false );

    //ensure this class won't think we have a map still
    m_tree  = NULL;
    m_focus = NULL;
    m_path  = QString::null;

    m_map.invalidate( b );
    if( b ) update();

    //tell rest of Filelight
    emit invalidated( url );
  }
}


//not responsibility of create() to invalidate first, if you don't it's your risk
void KRadialMap::create( const Directory *tree )
{
  if( tree != NULL )
  {
    //generate the filemap image
    m_map.make( tree );

    setMouseTracking( true );
  }

  m_tree = tree;
  m_path = fullPath( tree );

  //tell rest of Filelight
  emit created( m_tree );
}


void KRadialMap::createFromCache( const Directory *tree )
{
  //no scan was necessary, use cached tree, however we MUST still emit invalidate
  invalidate( false );
  create( tree );
}


void KRadialMap::slotPostMouseEvent()
{
  QMouseEvent me( QEvent::MouseMove, mapFromGlobal( QCursor::pos() ), Qt::NoButton, Qt::NoButton );
  QApplication::sendEvent( this, &me );
}


void KRadialMap::slotResizeTimeout() //always singleshot
{
  if( m_tree != NULL )
    m_map.make( m_tree, true );
  update();
}


void KRadialMap::refresh( int filth )
{
  if( !m_map.isNull() )
  {
    switch( filth ) {
    case 1:
      m_map.make( m_tree, true ); //true means refresh only
      break;

    case 2:
      m_map.aaPaint();
      break;

    case 3:
      m_map.colorise(); //intentional continue
    case 4:
      m_map.paint();

    default:
      break;
    }

    update();
  }
}


//**** these two slots could be one slot with a QSignalMapper handling the parameter
//all can crash if m_focus is NULL, but none at this point can be called if m_focus is NULL
void KRadialMap::slotKonqiHere()
{
  if( KRun::runCommand( "kfmclient openURL " + fullPath( m_focus->file() ) ) == 0 )
    KMessageBox::sorry( this, i18n( "Filelight could not start an instance of Konqueror." ) );
}
void KRadialMap::slotKonsoleHere()
{
  if( KRun::runCommand( "konsole --workdir " + fullPath( m_focus->file() ) ) == 0 )
    KMessageBox::sorry( this, i18n( "Filelight could not start an instance of Konsole." ) );
}
void KRadialMap::slotRun()
{
  (void) new KRun( fullPath( m_focus->file() ) );
}
void KRadialMap::slotCenterHere()
{
  KURL url; url.setPath( fullPath( m_focus->file() ) );
  emit activated( url );

  createFromCache( (Directory *)m_focus->file() );
}

void KRadialMap::slotZoomIn()
{
  if( m_map.m_visibleDepth > MIN_RING_DEPTH )
  {
    --m_map.m_visibleDepth;
    m_map.make( m_tree );
    Gsettings.defaultRingDepth = m_map.m_visibleDepth;
    update();
  }
}
void KRadialMap::slotZoomOut()
{
  ++m_map.m_visibleDepth;
  m_map.make( m_tree );
  if( m_map.m_visibleDepth > Gsettings.defaultRingDepth )
    Gsettings.defaultRingDepth = m_map.m_visibleDepth;
  update();
}

#include "kradialmap.moc"
