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

#include <qapplication.h> //setupActions()
#include <qevent.h>       //customEvent()
#include <qtimer.h>       //singleShot timers
#include <qstring.h>
#include <qcombobox.h>    //slotUpdateInterface()
#include <qlineedit.h>

#include <kapp.h>          //KDE_VERSION

#include <kaction.h>
#include <kaccel.h>        //keyboard shortcuts
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <ksimpleconfig.h> //m_config
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kurl.h>
#include <kdirselectdialog.h> //slotScanDirectory
#include <kcombobox.h>      //locationbar
#include <kurlcompletion.h> //locationbar
#include <kcursor.h>        //interface updates in startScan()

#include "define.h"     //for fullPath()
#include "filetree.h"
#include "filelight.h"
#include "canvas.h"
//#include "filemap.h"    //showSettings()
#include "scanthread.h"
#include "settings.h"
#include "settingsdlg.h"
#include "scanbox.h"
#include "historyaction.h"


Settings   Gsettings;   //global options struct


/**********************************************
  CONSTRUCTOR/DESTRUCTOR
 **********************************************/

Filelight::Filelight() : KMainWindow( 0, "filelight" ), m_bool( true )
{
  m_scanThread = new ScanThread;
  ScanThread::readMounts();
  ScanThread::appHandle = this;

  m_config = new KSimpleConfig( "filelightrc" );
  Gsettings.useKConfig( m_config );
  m_settingsDialog = new SettingsDlg( this, "settings_dialog" ); //will read our application settings from the config into the global settings structure

  m_canvas = new FilelightCanvas( this, "canvas" );
  setCentralWidget( m_canvas );
  
  setStandardToolBarMenuEnabled( true );
  
  setupStatusBar();
  setupActions();
  createGUI( "filelightui.rc" );

#if KDE_VERSION >= 0x030103
  m_config->setGroup( "general" );
  m_combo->setHistoryItems( m_config->readPathListEntry( "comboHistory" ) );
#endif
  
  connect( m_canvas, SIGNAL( invalidated( const QString & ) ), this, SLOT( slotUpdateHistories( const QString & ) ) );
  connect( m_canvas, SIGNAL( created( const Directory * ) ), this, SLOT( slotUpdateInterface( const Directory * ) ) );
  connect( m_canvas, SIGNAL( mouseOverSegment( const QString& ) ), statusBar(), SLOT( message( const QString & ) ) );
  connect( m_canvas, SIGNAL( mouseOverNothing() ), statusBar(), SLOT( clear() ) );

  connect( this, SIGNAL( scanStarted() ), m_status[1], SLOT( clear() ) );
  connect( this, SIGNAL( scanStarted() ), m_canvas, SLOT( invalidate() ) );
  connect( this, SIGNAL( scanUnrequired( const Directory * ) ), m_canvas, SLOT( createFromCache( const Directory * ) ) );
  connect( this, SIGNAL( scanFinished( const Directory * ) ), m_canvas, SLOT( create( const Directory * ) ) );
  connect( this, SIGNAL( scanFailed( const QString & ) ), m_canvas, SLOT( create( const QString & ) ) );
  connect( this, SIGNAL( scanFailed( const QString & ) ), m_combo, SLOT( clearEdit() ) );

//**** both names suck
  connect( m_settingsDialog, SIGNAL( dirtyCanvas( int ) ), m_canvas, SLOT( refresh( int ) ) );
  connect( m_settingsDialog, SIGNAL( treeCacheInvalidated() ), this, SLOT( slotClearCache() ) );

//  setAutoSaveSettings(); //**** didn't work
  applyMainWindowSettings( m_config, "window" );
  if( !m_config->hasGroup( "window" ) )
    resize( 600, 440 );
}


Filelight::~Filelight()
{
  delete m_config;

  if( m_scanThread->running() )
  {
    kdDebug( 0 ) << "Stopping scan-thread..\n";
    ScanThread::haltScans();
    m_scanThread->wait();
  }

  delete m_scanThread;
}


void Filelight::setupStatusBar()
{
  KStatusBar *statusbar = statusBar();

  m_status[0] = new QLabel( this, "status_message");
  m_status[1] = new QLabel( this, "status_files");

  m_status[0]->setIndent( 4 );
  m_status[0]->setText( i18n( "Scan to begin...")  );

  ScanProgressBox *progress = new ScanProgressBox( this, "progress_box" ); //see scanbox.h
    
  statusbar->addWidget( m_status[0], 1, false );
  statusbar->addWidget( m_status[1], 0, false );
  statusbar->addWidget( progress,    0, true );

  progress->hide(); //hide() here because add() (above) calls show() *rolls eyes*
  
  connect( this, SIGNAL( scanStarted() ), progress, SLOT( start() ) );
  connect( this, SIGNAL( scanFinished( const Directory * ) ), progress, SLOT( stop() ) );
  connect( this, SIGNAL( scanFailed( const QString & ) ), progress, SLOT( stop() ) );
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
  new KAction( i18n( "Scan &Home Directory" ), "folder_home", CTRL+Key_Home, this, SLOT( slotScanHomeDirectory() ), actionCollection(), "scan_home" );
  new KAction( i18n( "Scan &Root Directory" ), "folder_red_side", 0, this, SLOT( slotScanRootDirectory() ), actionCollection(), "scan_root" );
  m_recentHistory = new KRecentFilesAction( i18n( "&Recent Scans" ), 0, actionCollection(), "scan_recent", 8 );
  KAction *rescan = new KAction( i18n( "Rescan" ), "reload", KStdAccel::shortcut( KStdAccel::Reload ), this, SLOT( slotRescan() ), actionCollection(), "scan_rescan" );
  KAction *stop   = new KAction( i18n( "Stop" ), "stop", Qt::Key_Escape, this, SLOT( stopScan() ), actionCollection(), "scan_stop" );
  KStdAction::quit( this, SLOT( close() ), actionCollection() );

//view
  KStdAction::zoomIn( m_canvas, SLOT( slotZoomIn() ), actionCollection() );
  KStdAction::zoomOut( m_canvas, SLOT( slotZoomOut() ), actionCollection() );
  
//go
  KAction *up     = KStdAction::up( this, SLOT( slotUp() ), actionCollection() );
  m_backHistory    = new HistoryAction( i18n( "&Back" ), "back", KStdAccel::shortcut( KStdAccel::Back ), this, SLOT( slotBack() ), actionCollection(), "go_back" );
  m_forwardHistory = new HistoryAction( i18n( "&Forward" ), "forward", KStdAccel::shortcut( KStdAccel::Forward ), this, SLOT( slotForward() ), actionCollection(), "go_forward" );

//settings      
  KStdAction::preferences( m_settingsDialog, SLOT( show() ), actionCollection() );
//  KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
//  KStdAction::keyBindings(this, SLOT( slotConfigureKeyBindings()), actionCollection());


  m_recentHistory->loadEntries( m_config );
  connect( m_recentHistory, SIGNAL( urlSelected( const KURL& ) ), this, SLOT( slotScanUrl( const KURL& ) ) );
  combo->setAutoSized( true ); //**** what does this do?
  connect( m_combo, SIGNAL( returnPressed() ), this, SLOT( slotComboScan() ) );

      up->setEnabled( false );
    stop->setEnabled( false );
  rescan->setEnabled( false );
  //history actions are disabled in their ctors
}


bool Filelight::queryExit()
{
  saveMainWindowSettings( m_config, "window" );
  m_recentHistory->saveEntries( m_config );
#if KDE_VERSION >= 0x030103
  m_config->setGroup( "general" );
  //**** should be pathListEntry! but this works.. must be overide
  m_config->writePathEntry( "comboHistory", m_combo->historyItems() );
#endif  
  m_config->sync();

  return true;
}



void Filelight::saveProperties( KConfig *config )
{
#if KDE_VERSION >= 0x030103
  config->writePathEntry( "backHistory", m_backHistory->history() );
  config->writePathEntry( "forwardHistory", m_forwardHistory->history() );
#endif
  config->writeEntry( "currentMap", m_canvas->path() );
}

void Filelight::readProperties( KConfig *config )
{
#if KDE_VERSION >= 0x030103
//this is bugfree even if no session information was saved
  m_backHistory->setHistory( config->readPathListEntry( "backHistory" ) );
  m_forwardHistory->setHistory( config->readPathListEntry( "forwardHistory" ) );
  m_bool = false; //stop forward getting cleared on scan complete
#endif
  startScan( config->readEntry( "currentMap", QString::null ) );
}



/**********************************************
  SLOTS
 **********************************************/


void Filelight::slotScanDirectory()
{
  //**** idea is to set the path to the last scanned directory. Below doesn't work if scan cancelled/failed etc.
  QString s = m_canvas->path();
  if( s == QString::null )
    s = getenv( "HOME" ); //using getenv is cheap //**** so is QDir or whatever it is

  KURL url = KDirSelectDialog::selectDirectory( s, false, this, i18n( "Scan Directory") );

  if( !url.isEmpty() ) {
    m_recentHistory->addURL( url ); //**** what if protocol not good, path bad, etc.
    slotScanUrl( url );
  }
}
 
void Filelight::slotScanHomeDirectory()  { slotScanUrl( getenv( "HOME" ) ); }
void Filelight::slotScanRootDirectory()  { slotScanUrl( "/" ); }
void Filelight::slotUp()                 { slotScanUrl( KURL( m_canvas->path() ).upURL() ); }
void Filelight::slotRescan()             { startScan( m_canvas->path(), true ); }

void Filelight::slotScanUrl( const KURL &scanUrl ) //don't call for rescans
{
  //**** error message appears when not absolute path too
  if( scanUrl.protocol() != "file" )
    KMessageBox::sorry( this, i18n( "Sorry, Filelight only supports the file protocol currently. Mail the author to voice your frustration." ));
  else
    startScan( validateScan( scanUrl.path( 1 ) ) );
}


void Filelight::slotComboScan()
{
  /*
  int i = path.find( "*" );
  if( i != -1 )
  {
    m_canvas->setGlob( submission.mid( i ) );
    path.remove( i, path.length() );
  }
*/

  QString path = validateScan( m_combo->lineEdit()->text() );

  if( path != QString::null ) {
    if( path == m_canvas->path() ) //then force rescan
    {
      m_recentHistory->addURL( path );
      startScan( path, true );
    }
    else //then normal scan
    {
      m_combo->addToHistory( path );
      m_recentHistory->addURL( path );
      startScan( path );
    }
  }
}


void Filelight::slotClearCache()
{
  ScanThread::treeCache.empty();

  //**** make a new signal cacheInvalidated() that causes the canvas to invalidate
  //  ** make canvas invalidate() check it's validness before it goes to the trouble (or at least avoid CPU intensive desaturation if it thinks it's not valid)
  slotRescan();
}



/**********************************************
  STARTSCAN()
 **********************************************/

#include <sys/stat.h>
 
const QString Filelight::validateScan( const QString &submission )
{
  if( submission != QString::null && !submission.isEmpty() )
  {
    QString path = submission.stripWhiteSpace();
    struct stat statbuf;

    if( path[0] != '/' )
      path.prepend( '/' );
    if( !path.endsWith( "/" ) ) //**** slow
      path.append( '/' );

    if( stat( path, &statbuf ) == 0 )
      if( S_ISDIR( statbuf.st_mode ) )
        return path;
    
    statusBar()->message( i18n( "%1 is not a valid directory" ).arg( path ), 4000 );
  }

  return QString::null;
}

 
void Filelight::stopScan()
{
  m_status[0]->setText( i18n( "Aborting scan..." ) );
  ScanThread::haltScans();
}


void Filelight::startScan( const QString &path, bool forceScan )
{
  //**** eventually use a scanmanager class
  kdDebug( 0 ) << "Scan requested for: " << path << "\n";

  if( path == QString::null ) return;
  
  if( m_scanThread->running() ) { //shouldn't happen, but lets prevent mega-disasters just in case eh?
     kdWarning( 0 ) << "Filelight attempted to run 2 scans concurrently!\n";
     return;
  }

  /* CHECK IF ALREADY SCANNED
   * eg:
   *   user wants to scan:       /usr/local/
   *   user has already scanned: /usr/
   * we find /usr/ and search for the tree we want
   *
   * IN EVENT OF RESCAN
   * we store the Directory* to old tree but we still scan, if user cancels
   * we put the old tree back in the cache, if he doesn't we clear the store.
   */

  for( Iterator<Directory> it = ScanThread::treeCache.iterator();
       it != ScanThread::treeCache.end();
       ++it )
  {
    QString storedPath( (*it)->name() );

    if( path.startsWith( storedPath ) ) //then already scanned
    {
      //lets find a pointer to the branch that was requested

      kdDebug( 0 ) << "Cache-hit: " << storedPath << "!\n";

      QStringList list = QStringList::split( "/", path.mid( storedPath.length() ) );
      Directory *d = *it;
      Iterator<File> jt;

      while( !list.isEmpty() && d != NULL ) //if NULL we have got lost so abort!!
      {
        jt = d->iterator();

        const Link<File> *end = d->end();
        QString s = list.first() + "/";

        for( d = NULL; jt != end; ++jt )
          if( s == (*jt)->name() ) {
            d = (Directory *)*jt;
            break;
          }

        list.pop_front();
      }

      if( d != NULL && !forceScan ) {

        //we found a completed tree, thus no need to scan

          kdDebug( 0 ) << "Found requested tree :)\n";
          emit scanUnrequired( d );
          return;
      }
      else if( d != NULL && forceScan ) { //rescans

        //here we are deliberately breaking a cached tree in half
        //this is ok as long as we put the other half in the cache too
        //we will be doing that, when the scan is completed or cancelled

          kdDebug( 0 ) << "Scan forced\n";

          //it points to the original directory from cache
          //jt points to the thing we want (unless it already did, then jt == NULL)
          
          if( !jt.isNull() ) { //**** SUCKS! 
            //**** would use transferTo, but jt is <File> and scannedTrees<Dir> so incompatable
            jt.remove();

            //we can't have half trees in the cache so we must delete the whole tree
            // = unacceptable so I need to develop a post scan integrity check for the
            //   cache
            delete *it;
          }
          else //then it was pointing to what we wanted to start with!
            it.remove();
          

          m_scanThread->scannedTrees.append ( d );
          break;
      }
      else {

        //something is wrong, we couldn't find the directory we were expecting

          kdError( 0 ) << "Didn't find " << path << " in the cache!\n";
          delete it.remove(); //safest to get rid of it
          break; //do a full scan
      }
    }
  }
    
  //**** eventually split here?
  
  //start seperate m_scanMenu thread, control is immediately returned
  ScanThread::fileCounter = 0;
  ScanThread::haltScan    = false;
  m_scanThread->path       = path;

  //tell other parts of Filelight that a scan has started
  //** don't have race condition, do emit this signal first!!
  emit scanStarted();

  m_scanThread->start();

  //interface amendments
  //**** make this a slot called by emit scanStarted when you implement scanmanager
  QString qs = QString( i18n( "Scanning: %1" ).arg( path ) );

  QApplication::setOverrideCursor( KCursor::workingCursor() );
  stateChanged( "scan_started" );
  m_combo->clearFocus(); //**** KDE/Qt should do this fir you! bug?
  setCaption( qs );
  m_status[0]->setText( qs );
  m_status[1]->clear();
}


void Filelight::slotBack()
{
//**** add all this to the class so you don't have to implement here too
  m_bool = false;

  QString s = m_backHistory->pop();
  m_forwardHistory->push( m_canvas->path() );
  startScan( s );
}

void Filelight::slotForward()
{
  m_bool = false;

  QString s = m_forwardHistory->pop();
  m_backHistory->push( m_canvas->path() );
  startScan( s );
}


//map's been invalidated, keep a record of past
void Filelight::slotUpdateHistories( const QString &path )
{
  if( m_bool )
  {
    m_forwardHistory->clear();
    m_backHistory->push( path );
  }
  else
    m_bool = true;
}


//make interface reflect new map
void Filelight::slotUpdateInterface( const Directory *tree )
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
      //**** also there _must_ be a better way to do this! (ideally set
      //     before scan so we don't set twice (as is set if recentHistory is clicked directly)
      //     and then unset on failure
      //**** also you need to remove the file: when you support more than just file and then handle everything by url (will be interesting to implement :)
      m_recentHistory->setCurrentItem( m_recentHistory->items().findIndex( newPath.prepend( "file:" ) ) );
  }

  QApplication::restoreOverrideCursor();

//recent scans menu
//**** do I need to check that there are entries?
//**** how can I update the recent history's tick selection?

}


void Filelight::customEvent( QCustomEvent * e )
{
  switch( e->type() ) {
  case 65433: //scan succeeded
    {
      const Directory* const tree = static_cast<scanCompleteEvent*>(e)->tree();

      if( tree->isEmpty() )
      {
          QString s = tree->name();
          m_status[0]->setText( i18n( "No files found in: %1" ).arg( s ) );
          emit scanFailed( s );
          //delete tree; //deletion violates the cache

      } else {

          //inform UI that the scan is complete
          emit scanFinished( tree );
      }

    } break;

  case 65434: //scan failed, we can implement error messages into this

    m_status[0]->setText( i18n( "Scan failed/aborted!" ) );
    emit scanFailed( static_cast<scanFailedEvent*>(e)->path() );

  default: break;
  }
      
}
