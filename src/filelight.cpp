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

#include <kapplication.h>
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
#include <ksqueezedtextlabel.h>

//new stuff
#include <klibloader.h>

#include "filelight.h"
#include "filelightpart.h"
#include "settings.h"
#include "scanbox.h"
#include "historyaction.h"


/**********************************************
  CONSTRUCTOR/DESTRUCTOR
 **********************************************/

Filelight::Filelight()
  : m_part( new FilelightPart( this, "radialmap", this, "filelight_part", "" ) )
{
    setCentralWidget( m_part->widget() );
    setStandardToolBarMenuEnabled( true );
    setupStatusBar();
    setupActions();

/*
    factory = KLibLoader::self()->factory( "libfilelight" );
    if( factory )
    {
        // Create the part
        m_part = (KParts::ReadOnlyPart *)factory->create( this, "filelightpart", "KParts::ReadOnlyPart" );
        // Set the main widget
        setCentralWidget( m_part->widget() );
        // Integrate its GUI
        createGUI( m_part );
    }
    else
    {
       KMessageBox::error( this, "No FilelightPart found !" );
    }
*/


  createGUI( m_part );
  stateChanged( "scan_failed" );

  KConfig *config = KGlobal::config();

  config->setGroup( "general" );
  m_combo->setHistoryItems( config->readPathListEntry( "comboHistory" ) );

  applyMainWindowSettings( config, "window" );

  connect( m_part, SIGNAL( started( KIO::Job * ) ), this, SLOT( scanStarted() ) );
  connect( m_part, SIGNAL( completed() ), this, SLOT( scanCompleted() ) );
  connect( m_part, SIGNAL( canceled( const QString & ) ), this, SLOT( scanFailed( const QString & ) ) );
}


Filelight::~Filelight()
{}


void Filelight::setupStatusBar()
{
  KStatusBar *statusbar = statusBar();

  m_status[0] = new KSqueezedTextLabel( this, "status_message" );
  m_status[1] = new KSqueezedTextLabel( this, "status_files" );

  m_status[0]->setIndent( 4 );
  m_status[0]->setText( i18n( "Scan to begin..." ) );

  ScanProgressBox *progress = new ScanProgressBox( this, "progress_box" ); //see scanbox.h

  statusbar->addWidget( m_status[0], 1, false );
  statusbar->addWidget( m_status[1], 0, false );
  statusbar->addWidget( progress,    0, true );

  m_status[0]->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );
  m_status[1]->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );

  m_status[1]->hide(); //hide() here because add() (above) calls show() *rolls eyes*
  progress->hide();

//FIXME  connect( m_manager, SIGNAL( started( const QString & ) ), progress, SLOT( start() ) );
//FIXME  connect( m_manager, SIGNAL( succeeded( const Directory * ) ), progress, SLOT( stop() ) );
//FIXME  connect( m_manager, SIGNAL( failed( const QString &, ScanManager::ErrorCode ) ), progress, SLOT( stop() ) );

  connect( m_part , SIGNAL( newHoverFilename( const QString & ) ),  this, SLOT( newHoverFilename ( const QString & ) ) );
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
  new KAction( i18n( "Clear Location Bar" ), KApplication::reverseLayout() ? "clear_left" : "locationbar_erase", 0, m_combo, SLOT( clearEdit() ), actionCollection(), "clear_location" );
  new KAction( i18n( "Go" ), "key_enter", 0, m_combo, SIGNAL( returnPressed() ), actionCollection(), "go" );

//scan
  KStdAction::open( this, SLOT( slotScanDirectory() ), actionCollection(), "scan_directory" )->setText( i18n( "&Scan Directory..." ) );
  new KAction( i18n( "Scan &Home Directory" ), "folder_home", CTRL+Key_Home, this, SLOT( slotScanHomeDirectory() ), actionCollection(), "scan_home" );
  new KAction( i18n( "Scan &Root Directory" ), "folder_red", 0, this, SLOT( slotScanRootDirectory() ), actionCollection(), "scan_root" );
  m_recentScans = new KRecentFilesAction( i18n( "&Recent Scans" ), 0, actionCollection(), "scan_recent", 8 );
  new KAction( i18n( "Rescan" ), "reload", KStdAccel::reload(), this, SLOT( slotRescan() ), actionCollection(), "scan_rescan" );
  new KAction( i18n( "Stop" ), "stop", Qt::Key_Escape, this, SLOT( slotAbortScan() ), actionCollection(), "scan_stop" );
  KStdAction::quit( this, SLOT( close() ), actionCollection() );

//go
  KStdAction::up( this, SLOT( slotUp() ), actionCollection() );
  m_histories = new HistoryCollection( actionCollection(), this, "history_collection" );

//settings
  KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
//  KStdAction::keyBindings(this, SLOT( slotConfigureKeyBindings()), actionCollection());

  m_recentScans->loadEntries( KGlobal::config() );
  combo->setAutoSized( true ); //**** what does this do?

  connect( m_recentScans, SIGNAL( urlSelected( const KURL& ) ), this, SLOT( slotScanUrl( const KURL& ) ) );
  connect( m_combo, SIGNAL( returnPressed() ), this, SLOT( slotComboScan() ) );
  connect( m_histories, SIGNAL( activated( const KURL & ) ), this, SLOT( slotScanUrl( const KURL & ) ) );
}


bool Filelight::queryExit()
{
  KConfig *config = KGlobal::config();

  saveMainWindowSettings( config, "window" );
  m_recentScans->saveEntries( config );
  config->setGroup( "general" );
  config->writePathEntry( "comboHistory", m_combo->historyItems() );
  config->sync();

  return true;
}


/**********************************************
  SESSION MANAGEMENT
 **********************************************/

void Filelight::saveProperties( KConfig *config )
{
  m_histories->save( config );
  config->writeEntry( "currentMap", m_part->url().path() );
}

void Filelight::readProperties( KConfig *config )
{
  m_histories->restore( config );
  m_part->openURL( config->readEntry( "currentMap", "" ) );
}



/**********************************************
  SLOTS
 **********************************************/

void Filelight::editToolbars()
{
  //**** personally I feel this should be handled by KMainWindow, so pay attention and see if it gets implemented that way
  saveMainWindowSettings( KGlobal::config(), "window" );
  KEditToolbar dlg( factory(), this );
  connect(&dlg, SIGNAL( newToolbarConfig() ), this, SLOT( slotNewToolbarConfig() ));
  dlg.exec();
}

void Filelight::slotNewToolbarConfig()
{
  //createGUI( 0L );
  createGUI( m_part );
  applyMainWindowSettings( KGlobal::config(), "window" );
}


void Filelight::slotScanDirectory()
{
  const KURL url = KDirSelectDialog::selectDirectory( m_part->url().path(), false, this );

  if( !url.isEmpty() )
    slotScanUrl( url );
}

void Filelight::slotScanHomeDirectory() { slotScanUrl( getenv( "HOME" ) ); }
void Filelight::slotScanRootDirectory() { slotScanUrl( "/" ); }
void Filelight::slotUp()                { slotScanUrl( m_part->url().upURL() ); }
void Filelight::slotRescan()            { slotScanUrl( m_part->url() ); }

void Filelight::slotComboScan()
{
  const QString path( m_combo->lineEdit()->text() );

  if( slotScanUrl( path ) )
    m_combo->addToHistory( path ); //add if path was sane
}


bool Filelight::slotScanUrl( const KURL &url )
{
    return m_part->openURL( url );
}


void Filelight::slotAbortScan()
{
  actionCollection()->action( "scan_stop" )->setEnabled( false );
  m_status[0]->setText( "Aborting scan..." );

  m_part->closeURL();
}




void Filelight::scanStarted()
{
    kdDebug() << "started\n";

  //interface amendments
  QString qs = i18n( "Scanning: %1" ).arg( m_part->url().path() );
  stateChanged( "scan_started" );
  m_combo->clearFocus(); //**** KDE/Qt should do this for you! bug?
//  setCaption( qs );
  m_status[0]->setText( qs );
  m_status[1]->hide();
}


void Filelight::scanFailed( const QString &err )
{
    KMessageBox::sorry( this, err );

    stateChanged( "scan_failed" );
    actionCollection()->action( "go_up" )->setText( i18n( "Up" ) );

    m_status[0]->setText( i18n( "The scan did not complete" ) );
}


void Filelight::scanCompleted()
{
    KAction *goUp = actionCollection()->action( "go_up" );
    QString path = m_part->url().path( 1 );

    QLineEdit *edit = m_combo->lineEdit();
    KURL url( m_part->url() );

    stateChanged( "scan_complete" );

    if( edit->text() != path )
    edit->setText( path );


    //FIXME the part handles all this
    m_status[0]->setText( i18n( "Showing: %1" ).arg( path ) );
    //m_status[1]->setText( i18n( "Files: %1" ).arg( KGlobal::locale()->formatNumber( tree->fileCount(), 0 ) ) );
    m_status[1]->show();

    if( path == "/" )
    {
        goUp->setEnabled( false );
        goUp->setText( i18n( "Up" ) );
    }
    else
        goUp->setText( i18n( "Up: %1" ).arg( url.upURL().path( 1 ) ) );

    m_recentScans->addURL( url ); //FIXME doesn't set the tick
}


void Filelight::newHoverFilename(const QString & fullpath)
{
    m_status[0]->setText( fullpath );
}


#include "filelight.moc"
