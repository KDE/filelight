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

//#include <stdlib.h> //getenv()

#include <qstring.h>

#include <kaction.h>
#include <kaccel.h>        //keyboard shortcuts
#include <kedittoolbar.h>  //for editToolbar dialog
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <ksimpleconfig.h> //m_config
#include <klocale.h>
#include <kurl.h>
#include <kdirselectdialog.h> //slotScanDirectory
#include <kcombobox.h>        //locationbar
#include <kurlcompletion.h>   //locationbar
#include <kcursor.h>          //access to KCursors
#include <kmessagebox.h>      //scanFailed()

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

Filelight::Filelight() : KMainWindow( 0, "filelight" ),
    m_config( /*new KConfig( "filelightrc" )*/ KGlobal::config() ),
    m_manager( new ScanManager( this, "scan_manager" ) ),
    m_canvas( new FilelightCanvas( statusBar(), this, "canvas" ) )
{
  Gsettings.readSettings( m_config ); //no class can have references to Gsettings in its ctor

  ScanManager::readMounts();

  setCentralWidget( m_canvas );
  setStandardToolBarMenuEnabled( true );
  
  setupStatusBar();
  setupActions();
  createGUI();
  stateChanged( "scan_failed" );

#if KDE_VERSION >= 0x030103
  m_config->setGroup( "general" );
  m_combo->setHistoryItems( m_config->readPathListEntry( "comboHistory" ) );
#endif
  
  connect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_histories, SLOT( push( const KURL & ) ) );
  connect( m_canvas, SIGNAL( created( const Directory * ) ), this, SLOT( newMapCreated( const Directory * ) ) );

//**** should I handle scan-related signals with events instead?
//     probably since you don't depend on immediate execution of any of this code
//     also this could all be part of one event
  connect( m_manager, SIGNAL( started( const QString & ) ), m_canvas, SLOT( invalidate() ) );
  connect( m_manager, SIGNAL( started( const QString & ) ), this, SLOT( scanStarted( const QString & ) ) );
  connect( m_manager, SIGNAL( cached( const Directory * ) ), m_canvas, SLOT( createFromCache( const Directory * ) ) );
  connect( m_manager, SIGNAL( succeeded( const Directory * ) ), m_canvas, SLOT( create( const Directory * ) ) );
  connect( m_manager, SIGNAL( failed( const QString &, ScanManager::ErrorCode ) ), this, SLOT( scanFailed( const QString &, ScanManager::ErrorCode ) ) );
  connect( m_manager, SIGNAL( aborted() ), this, SLOT( scanAborted() ) );

  connect( m_manager, SIGNAL( cacheInvalidated() ), m_canvas, SLOT( invalidate() ) );

  applyMainWindowSettings( m_config, "window" );
}


Filelight::~Filelight()
{
//  delete m_config;
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

  m_status[1]->hide(); //hide() here because add() (above) calls show() *rolls eyes*
  progress->hide();
  
  connect( m_manager, SIGNAL( started( const QString & ) ), progress, SLOT( start() ) );
  connect( m_manager, SIGNAL( succeeded( const Directory * ) ), progress, SLOT( stop() ) );
  connect( m_manager, SIGNAL( failed( const QString &, ScanManager::ErrorCode ) ), progress, SLOT( stop() ) );
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
  m_recentScans = new KRecentFilesAction( i18n( "&Recent Scans" ), 0, actionCollection(), "scan_recent", 8 );
  new KAction( i18n( "Rescan" ), "reload", KStdAccel::reload(), this, SLOT( slotRescan() ), actionCollection(), "scan_rescan" );
  new KAction( i18n( "Stop" ), "stop", Qt::Key_Escape, m_manager, SLOT( abort() ), actionCollection(), "scan_stop" );
  KStdAction::quit( this, SLOT( close() ), actionCollection() );

//view
  KStdAction::zoomIn( m_canvas, SLOT( slotZoomIn() ), actionCollection() );
  KStdAction::zoomOut( m_canvas, SLOT( slotZoomOut() ), actionCollection() );

//go
  KStdAction::up( this, SLOT( slotUp() ), actionCollection() );
  m_histories = new HistoryCollection( actionCollection(), this, "history_collection" );

//settings
  KStdAction::preferences( this, SLOT( showSettings() ), actionCollection() );
  KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
//  KStdAction::keyBindings(this, SLOT( slotConfigureKeyBindings()), actionCollection());

  m_recentScans->loadEntries( m_config );
  combo->setAutoSized( true ); //**** what does this do?

  connect( m_recentScans, SIGNAL( urlSelected( const KURL& ) ), this, SLOT( slotScanUrl( const KURL& ) ) );
  connect( m_combo, SIGNAL( returnPressed() ), this, SLOT( slotComboScan() ) );
  connect( m_histories, SIGNAL( activated( const KURL & ) ), this, SLOT( slotScanUrl( const KURL & ) ) );
}


bool Filelight::queryExit()
{
  saveMainWindowSettings( m_config, "window" );
  m_recentScans->saveEntries( m_config );
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
  config->writeEntry( "currentMap", m_canvas->path() );
}

void Filelight::readProperties( KConfig *config )
{
  m_histories->restore( config );
  m_manager->start( config->readEntry( "currentMap", "" ) );
}



/**********************************************
  SLOTS
 **********************************************/

void Filelight::showSettings()
{
  SettingsDlg *dialog = new SettingsDlg( &Gsettings, this, "settings_dialog" );

  connect( dialog, SIGNAL( canvasIsDirty( int ) ), m_canvas, SLOT( refresh( int ) ) );
  connect( dialog, SIGNAL( mapIsInvalid() ), m_manager, SLOT( emptyCache() ) );

  dialog->show(); //deletes itself
}

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
  const KURL url = KDirSelectDialog::selectDirectory( m_canvas->path(), false, this );

  if( !url.isEmpty() )
    slotScanUrl( url );
}

void Filelight::slotScanHomeDirectory() { slotScanUrl( getenv( "HOME" ) ); }
void Filelight::slotScanRootDirectory() { slotScanUrl( "/" ); }
void Filelight::slotUp()                { slotScanUrl( KURL( m_canvas->path() ).upURL() ); }

bool Filelight::slotScanUrl( const KURL &url )
{
  if( url == KURL( m_canvas->path() ) )
  {
    slotRescan();
    return true;
  }
  else
    return m_manager->start( url );
}

void Filelight::slotRescan()
{
  //**** this is far from ideal. You lose the whole cache! FIXME!
  //this disconnection to prevent histories from being "updated"
  disconnect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_histories, SLOT( push( const KURL & ) ) );
  connect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_manager, SLOT( _start( const KURL & ) ) );
  m_manager->emptyCache(); //causes canvas to invalidate
  disconnect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_manager, SLOT( _start( const KURL & ) ) );
  connect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_histories, SLOT( push( const KURL & ) ) );
}


void Filelight::slotComboScan()
{
  const QString path( m_combo->lineEdit()->text() );

  if( slotScanUrl( path ) )
    m_combo->addToHistory( path ); //add if path was sane
}


//
//following two functions maintain the interface during/after scan operations

void Filelight::scanStarted( const QString &path )
{
  //interface amendments
  QString qs = i18n( "Scanning: %1" ).arg( path );
  QApplication::setOverrideCursor( KCursor::workingCursor() );
  stateChanged( "scan_started" );
  m_combo->clearFocus(); //**** KDE/Qt should do this for you! bug?
  setCaption( qs );
  m_status[0]->setText( qs );
  m_status[1]->hide();
}


void Filelight::scanFailed( const QString &path, ScanManager::ErrorCode err )
{
  QString s;

  switch( err )
  {
  case ScanManager::InvalidProtocol:
    s = i18n( "The URL protocol must be: file:/" );
    break;
  case ScanManager::InvalidUrl:
    s = i18n( "The URL is not valid: %1" ).arg( path );
    break;
  case ScanManager::RelativePath:
    s = i18n( "Filelight only accepts absolute paths, eg. /%1" ).arg( path );
    break;
  case ScanManager::NotFound:
    s = i18n( "Directory not found: %1" ).arg( path );
    break;
  case ScanManager::NoPermission:
    s = i18n( "Unable to enter: %1\nYou don't have access rights to this location." ).arg( path );
    break;

  default:
    newMapCreated( NULL ); //will disable the interface, only suitable for some errors
    return;
  }

  KMessageBox::sorry( this, s );
}


void Filelight::scanAborted()
{
  actionCollection()->action( "scan_stop" )->setEnabled( false );
  m_status[0]->setText( "Aborting scan..." );
}


void Filelight::newMapCreated( const Directory *tree )
{
  KAction *goUp = actionCollection()->action( "go_up" );
  QString path = fullPath( tree );

  if( tree == NULL ) //if no tree we keep a somewhat disabled interface
  {
      stateChanged( "scan_failed" );

      goUp->setText( i18n( "Up" ) );
      setCaption( "" );

      //**** shown for abort mostly, which sucks
      m_status[0]->setText( i18n( "The scan did not complete" ) );

  } else {

      QLineEdit *edit = m_combo->lineEdit();
      KURL url( path );

      stateChanged( "scan_complete" );

      if( edit->text() != path )
        edit->setText( path );

      setCaption( path );
      m_status[0]->setText( i18n( "Showing: %1" ).arg( path ) );
      m_status[1]->setText( i18n( "Files: %1" ).arg( KGlobal::locale()->formatNumber( tree->fileCount(), 0 ) ) );
      m_status[1]->show();

      if( path == "/" )
      {
        goUp->setEnabled( false );
        goUp->setText( i18n( "Up" ) );
      }
      else
        goUp->setText( i18n( "Up: %1" ).arg( url.upURL().path( 1 ) ) );

      m_recentScans->addURL( url ); //**** doesn't set the tick
  }

  QApplication::restoreOverrideCursor();
}


#include "filelight.moc"
