/***************************************************************************
                          filelight.cpp  -  description
                             -------------------
    begin                : Mon May 12 22:38:30 BST 2003
    copyright            : (C) 2003 by Max Howell
    email                : mh9193@bris.ac.uk
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

#include <stdlib.h> //getenv()

#include <qstring.h>

#include <kaction.h>
#include <kaccel.h>        //keyboard shortcuts
#include <kedittoolbar.h>  //for editToolbar dialog
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <ksimpleconfig.h> //m_config
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kdirselectdialog.h> //slotScanDirectory
#include <kcombobox.h>        //locationbar
#include <kurlcompletion.h>   //locationbar
#include <kcursor.h>          //access to KCursors

#include "define.h"     //for fullPath()
#include "filetree.h"
#include "filelight.h"
#include "canvas.h"
#include "scanmanager.h"
#include "settings.h"
#include "settingsdlg.h"
#include "scanbox.h"
#include "historyaction.h"


Settings Gsettings;   //global options struct


/**********************************************
  CONSTRUCTOR/DESTRUCTOR
 **********************************************/

Filelight::Filelight() : KMainWindow( 0, "filelight" )
{
  //**** some of these can be initialised with the class
  m_config = new KSimpleConfig( "filelightrc" );
  Gsettings.useKConfig( m_config );
  m_settingsDialog = new SettingsDlg( this, "settings_dialog" ); //will read our application settings from the config into the global settings structure

  m_canvas = new FilelightCanvas( this, "canvas" );
  setCentralWidget( m_canvas );

  //**** was crashing when canvas was after scan_manager, probably since cache was somehow linked to signature. This is bad.
  m_manager = new ScanManager( this, "scan_manager" );
  ScanManager::readMounts();
      
  setStandardToolBarMenuEnabled( true );
  
  setupStatusBar();
  setupActions();
  createGUI( "filelightui.rc" );
 // stateChanged( "scan_failed" ); //**** for some reason doing this didn't disable the zoom actions. Don't know why yet

#if KDE_VERSION >= 0x030103
  m_config->setGroup( "general" );
  m_combo->setHistoryItems( m_config->readPathListEntry( "comboHistory" ) );
#endif
  
  connect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_histories, SLOT( push( const KURL & ) ) );
  connect( m_canvas, SIGNAL( created( const Directory * ) ), this, SLOT( newMapCreated( const Directory * ) ) );
//**** would be better to just pass a pointer to the statusbar to canvas I feel
  connect( m_canvas, SIGNAL( mouseOverSegment( const QString& ) ), statusBar(), SLOT( message( const QString & ) ) );
  connect( m_canvas, SIGNAL( mouseOverNothing() ), statusBar(), SLOT( clear() ) );

//**** should I handle scan-related signals with events instead?
//     probably since you don't depend on immediate execution of any of this code
//     also this could all be part of one event
  connect( m_manager, SIGNAL( started( const QString & ) ), m_status[1], SLOT( clear() ) );
  connect( m_manager, SIGNAL( started( const QString & ) ), m_canvas, SLOT( invalidate() ) );
  connect( m_manager, SIGNAL( started( const QString & ) ), this, SLOT( scanStarted( const QString & ) ) );
  connect( m_manager, SIGNAL( cached( const Directory * ) ), m_canvas, SLOT( createFromCache( const Directory * ) ) );
  connect( m_manager, SIGNAL( succeeded( const Directory * ) ), m_canvas, SLOT( create( const Directory * ) ) );
  connect( m_manager, SIGNAL( failed( const QString & ) ), m_canvas, SLOT( create( const QString & ) ) );
  connect( m_manager, SIGNAL( failed( const QString & ) ), m_combo, SLOT( clearEdit() ) );
  connect( m_manager, SIGNAL( failed( const QString & ) ), m_combo, SLOT( clearEdit() ) );
  connect( m_manager, SIGNAL( failed( const QString & ) ), this, SLOT( scanFailed( const QString & ) ) );
  connect( m_manager, SIGNAL( cacheInvalidated() ), m_canvas, SLOT( invalidate() ) );

  connect( m_settingsDialog, SIGNAL( canvasIsDirty( int ) ), m_canvas, SLOT( refresh( int ) ) );
//execution order is arbituary, but both slots cause map invalidation, so it doesn't matter which one is called first :)
  connect( m_settingsDialog, SIGNAL( mapIsInvalid() ), this, SLOT( slotRescan() ) );
  connect( m_settingsDialog, SIGNAL( mapIsInvalid() ), m_manager, SLOT( emptyCache() ) );

  applyMainWindowSettings( m_config, "window" );
  //**** make a default config and then remove this
  //     also remove the default skipList bit as that is broken anyway
  if( !m_config->hasGroup( "window" ) )
    resize( 600, 440 );
}


Filelight::~Filelight()
{
  delete m_config;
}


void Filelight::setupStatusBar()
{
  KStatusBar *statusbar = statusBar();

  m_status[0] = new QLabel( this, "status_message" );
  m_status[1] = new QLabel( this, "status_files" );

  m_status[0]->setIndent( 4 );
  m_status[0]->setText( i18n( "Scan to begin..." ) );

  ScanProgressBox *progress = new ScanProgressBox( this, "progress_box" ); //see scanbox.h
    
  statusbar->addWidget( m_status[0], 1, false );
  statusbar->addWidget( m_status[1], 0, false );
  statusbar->addWidget( progress,    0, true );

  progress->hide(); //hide() here because add() (above) calls show() *rolls eyes*
  
  connect( m_manager, SIGNAL( started( const QString & ) ), progress, SLOT( start() ) );
  connect( m_manager, SIGNAL( succeeded( const Directory * ) ), progress, SLOT( stop() ) );
  connect( m_manager, SIGNAL( failed( const QString & ) ), progress, SLOT( stop() ) );
}

void Filelight::setupActions()
{
  m_combo = new KHistoryCombo( 0L, "history_combo" );
  m_combo->setCompletionObject( new KURLCompletion( KURLCompletion::DirCompletion ) );
  m_combo->setAutoDeleteCompletionObject( true );
  m_combo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
  m_combo->setDuplicatesEnabled( false );

//locationToolBar
  KWidgetAction *combo = new KWidgetAction( m_combo, i18n( "Location Bar" ), 0, 0, 0, actionCollection(), "location_bar" );
  new KAction( i18n( "Clear Location Bar" ), QApplication::reverseLayout() ? "clear_left" : "locationbar_erase", 0, m_combo, SLOT( clearEdit() ), actionCollection(), "clear_location" );
  new KAction( i18n( "Go" ), "key_enter", 0, m_combo, SIGNAL( returnPressed() ), actionCollection(), "go" );

//scan
  KStdAction::open( this, SLOT( slotScanDirectory() ), actionCollection(), "scan_directory" )->setText( i18n( "&Scan Directory..." ) );
  new KAction( i18n( "Scan &Home Directory" ), "gohome", CTRL+Key_Home, this, SLOT( slotScanHomeDirectory() ), actionCollection(), "scan_home" );
  new KAction( i18n( "Scan &Root Directory" ), "folder_red_side", 0, this, SLOT( slotScanRootDirectory() ), actionCollection(), "scan_root" );
  m_recentHistory = new KRecentFilesAction( i18n( "&Recent Scans" ), 0, actionCollection(), "scan_recent", 8 );
  KAction *reload = new KAction( i18n( "Rescan" ), "reload", KStdAccel::reload(), this, SLOT( slotRescan() ), actionCollection(), "scan_rescan" );
  KAction *stop   = new KAction( i18n( "Stop" ), "stop", Qt::Key_Escape, m_manager, SLOT( abort() ), actionCollection(), "scan_stop" );
  KStdAction::quit( this, SLOT( close() ), actionCollection() );

//view
  KStdAction::zoomIn( m_canvas, SLOT( slotZoomIn() ), actionCollection() )->setEnabled( false );
  KStdAction::zoomOut( m_canvas, SLOT( slotZoomOut() ), actionCollection() )->setEnabled( false );

//go
  KStdAction::up( this, SLOT( slotUp() ), actionCollection() )->setEnabled( false );
  m_histories = new HistoryCollection( actionCollection(), this, "history_collection" );

//settings      
  KStdAction::preferences( m_settingsDialog, SLOT( show() ), actionCollection() );
  KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
//  KStdAction::keyBindings(this, SLOT( slotConfigureKeyBindings()), actionCollection());

  m_recentHistory->loadEntries( m_config );
  combo->setAutoSized( true ); //**** what does this do?
  
  connect( m_recentHistory, SIGNAL( urlSelected( const KURL& ) ), this, SLOT( slotScanUrl( const KURL& ) ) );
  connect( m_combo, SIGNAL( returnPressed() ), this, SLOT( slotComboScan() ) );
  connect( m_histories, SIGNAL( activated( const KURL & ) ), this, SLOT( slotScanUrl( const KURL & ) ) );

  reload->setEnabled( false );
  stop->setEnabled( false );
}


bool Filelight::queryExit()
{
  saveMainWindowSettings( m_config, "window" );
  m_recentHistory->saveEntries( m_config );
#if KDE_VERSION >= 0x030103
  m_config->setGroup( "general" );
  m_config->writePathEntry( "comboHistory", m_combo->historyItems() );
#endif  
  m_config->sync();

  return true;
}


/**********************************************
  SESSION MANAGEMENT
 **********************************************/

void Filelight::saveProperties( KConfig *config )
{
  m_histories->save( config );
  //**** use KURLs
  config->writeEntry( "currentMap", m_canvas->path() );
}

void Filelight::readProperties( KConfig *config )
{
  m_histories->restore( config );
  //**** use KURLs
  m_manager->start( config->readEntry( "currentMap", QString::null ) );
}



/**********************************************
  SLOTS
 **********************************************/

void Filelight::editToolbars()
{
  //**** personally I feel this should be handles by KMainWindow, so pay attention and see if it gets implemented that way
  saveMainWindowSettings( m_config, "window" );
  KEditToolbar dlg( factory(), this );
  connect(&dlg, SIGNAL( newToolbarConfig() ), this, SLOT( slotNewToolbarConfig() ));
  dlg.exec();
}

void Filelight::slotNewToolbarConfig()
{
  createGUI();
  applyMainWindowSettings( m_config, "window" );
}

 
void Filelight::slotScanDirectory()
{
  //**** idea is to set the path to the last scanned directory. Below doesn't work if scan cancelled/failed etc.
  QString s = m_canvas->path();
  if( s == QString::null )
    s = getenv( "HOME" ); //using getenv is cheap //**** so is QDir or whatever it is

  const KURL url = KDirSelectDialog::selectDirectory( s, false, this, i18n( "Scan Directory") );

  if( !url.isEmpty() )
    m_manager->start( url );
}
 
void Filelight::slotScanHomeDirectory() { m_manager->start( getenv( "HOME" ) ); }
void Filelight::slotScanRootDirectory() { m_manager->start( "/" ); }
void Filelight::slotUp()                { m_manager->start( KURL( m_canvas->path() ).upURL() ); }
void Filelight::slotScanUrl( const KURL &url ) { m_manager->start( url ); }

void Filelight::slotRescan()
{
  //this disconnection to prevent histories from being "updated"
  disconnect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_histories, SLOT( push( const QString & ) ) );  
  connect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_manager, SLOT( start( const KURL & ) ) );
  m_canvas->invalidate();
  disconnect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_manager, SLOT( start( const KURL & ) ) );
  connect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_histories, SLOT( push( const QString & ) ) );  
}


void Filelight::slotComboScan()
{
  //**** m_canvas->path() is poo remove it if you can!
  const QString path( m_combo->lineEdit()->text() );

  if( !path.isEmpty() )
  {
    if( path == m_canvas->path() ) //then force rescan

      m_manager->start( path, true );

    else //then normal scan
    {
      m_combo->addToHistory( path );
      m_manager->start( path );
    }
  }
}


//
//following two functions maintain the interface during/after scan operations

void Filelight::scanStarted( const QString &path )
{
  //interface amendments
  QString qs = QString( i18n( "Scanning: %1" ).arg( path ) );
  QApplication::setOverrideCursor( KCursor::workingCursor() );
  stateChanged( "scan_started" );
  m_combo->clearFocus(); //**** KDE/Qt should do this for you! bug?
  setCaption( qs );
  m_status[0]->setText( qs );
  m_status[1]->clear();
}


void Filelight::newMapCreated( const Directory *tree )
{
  KAction *goUp = actionCollection()->action( "go_up" );

  if( tree == NULL ) //if no tree we keep a somewhat disabled interface
  {
      stateChanged( "scan_failed" );

      goUp->setText( i18n( "Up" ) );
      setCaption( "" );

  } else {

      QString newPath = fullPath( tree );
      QLineEdit *edit = m_combo->lineEdit();
            
      stateChanged( "scan_complete" );
      
      if( edit->text() != newPath )
        edit->setText( newPath );
                  
      setCaption( newPath );
      m_status[0]->setText( i18n( "Showing: %1" ).arg( newPath ) );
      m_status[1]->setText( i18n( "Files: %1" ).arg( KGlobal::locale()->formatNumber( tree->fileCount(), 0 ) ) );

      if( newPath != "/" ) {
        KURL url( newPath );
        goUp->setText( i18n( "Up: %1" ).arg( url.upURL().path( 1 ) ) );
      }

      //**** should be done on failure too?
      //     don't think so, but we do store failed paths in the canvas
      //**** also you need to remove the file: when you support more than just file and then handle everything by url (will be interesting to implement :)
      m_recentHistory->addURL( newPath );
//      m_recentHistory->setCurrentItem( m_recentHistory->items().findIndex( newPath.prepend( "file:" ) ) );
  }

  QApplication::restoreOverrideCursor();
}


void Filelight::scanFailed( const QString &path )
{
  //**** message _should_ be more specific, why did it fail?
  m_status[0]->setText( i18n( "Unable to scan %1" ).arg( path ) );
  newMapCreated( NULL );
}


#include "filelight.moc"
