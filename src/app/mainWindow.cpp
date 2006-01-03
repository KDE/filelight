//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include "mainWindow.h"
#include "part/part.h"
#include "historyAction.h"

#include <kaccel.h>           //KStdAccel namespace
#include <kaction.h>
#include <kapplication.h>     //setupActions()
#include <kcombobox.h>        //locationbar
#include <kconfig.h>
#include <kdirselectdialog.h> //slotScanDirectory
#include <kedittoolbar.h>     //for editToolbar dialog
#include <kkeydialog.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kshell.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <kurl.h>
#include <kurlcompletion.h>   //locationbar
#include <stdlib.h>           //std::exit()


namespace Filelight {

MainWindow::MainWindow() : KParts::MainWindow(), m_part( 0 )
{
    KLibFactory *factory = KLibLoader::self()->factory( "libfilelight" );

    if( !factory ) {
       KMessageBox::error( this, i18n("KDE could not find the Filelight Part, or the Filelight Part could not be started. Did you make install?") );
       //exit() seems to not exist inside the std namespace for some users!
       using namespace std;
       exit( 1 ); //don't use QApplication::exit() - it causes a crash
    }

    m_part = (Part *)factory->create( this, "part", "KParts::ReadOnlyPart" );

    setCentralWidget( m_part->widget() );
    setStandardToolBarMenuEnabled( true );
    setupActions();
    createGUI( m_part );

    stateChanged( "scan_failed" ); //bah! doesn't affect the parts' actions, should I add them to the actionCollection here?

    connect( m_part, SIGNAL(started( KIO::Job* )), SLOT(scanStarted()) );
    connect( m_part, SIGNAL(completed()), SLOT(scanCompleted()) );
    connect( m_part, SIGNAL(canceled( const QString& )), SLOT(scanFailed()) );

    //TODO test these
    connect( m_part, SIGNAL(canceled( const QString& )), m_histories, SLOT(stop()) );
    connect( BrowserExtension::childObject( m_part ), SIGNAL(openURLNotify()), SLOT(urlAboutToChange()) );

    KConfig* const config = KGlobal::config();
    config->setGroup( "general" );
    m_combo->setHistoryItems( config->readPathListEntry( "comboHistory" ) );
    applyMainWindowSettings( config, "window" );

    statusBar()->message( i18n( "Use the Scan-menu to begin..." ) );
}

inline void
MainWindow::setupActions() //singleton function
{
    KActionCollection *const ac = actionCollection();

    m_combo = new KHistoryCombo( this, "history_combo" );
    m_combo->setCompletionObject( new KURLCompletion( KURLCompletion::DirCompletion ) );
    m_combo->setAutoDeleteCompletionObject( true );
    m_combo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    m_combo->setDuplicatesEnabled( false );

    KStdAction::open( this, SLOT(slotScanDirectory()), ac, "scan_directory" );
    KStdAction::quit( this, SLOT(close()), ac );
    KStdAction::up( this, SLOT(slotUp()), ac );
    KStdAction::configureToolbars(this, SLOT(configToolbars()), ac);
    KStdAction::keyBindings(this, SLOT(configKeys()), ac);

    new KAction( i18n( "Scan &Home Directory" ), "folder_home", CTRL+Key_Home, this, SLOT(slotScanHomeDirectory()), ac, "scan_home" );
    new KAction( i18n( "Scan &Root Directory" ), "folder_red", 0, this, SLOT(slotScanRootDirectory()), ac, "scan_root" );
    new KAction( i18n( "Rescan" ), "reload", KStdAccel::reload(), m_part, SLOT(rescan()), ac, "scan_rescan" );
    new KAction( i18n( "Stop" ), "stop", Qt::Key_Escape, this, SLOT(slotAbortScan()), ac, "scan_stop" );
    new KAction( i18n( "Clear Location Bar" ), KApplication::reverseLayout() ? "clear_left" : "locationbar_erase", 0, m_combo, SLOT(clearEdit()), ac, "clear_location" );
    new KAction( i18n( "Go" ), "key_enter", 0, m_combo, SIGNAL(returnPressed()), ac, "go" );

    KWidgetAction *combo = new KWidgetAction( m_combo, i18n( "Location Bar" ), 0, 0, 0, ac, "location_bar" );
    m_recentScans = new KRecentFilesAction( i18n( "&Recent Scans" ), 0, ac, "scan_recent", 8 );
    m_histories = new HistoryCollection( ac, this, "history_collection" );

    ac->action( "scan_directory" )->setText( i18n( "&Scan Directory..." ) );
    m_recentScans->loadEntries( KGlobal::config() );
    combo->setAutoSized( true ); //FIXME what does this do?

    connect( m_recentScans, SIGNAL(urlSelected( const KURL& )), SLOT(slotScanUrl( const KURL& )) );
    connect( m_combo, SIGNAL(returnPressed()), SLOT(slotComboScan()) );
    connect( m_histories, SIGNAL(activated( const KURL& )), SLOT(slotScanUrl( const KURL& )) );
}

bool
MainWindow::queryExit()
{
    if( !m_part ) //apparently std::exit() still calls this function, and abort() causes a crash..
       return true;

    KConfig* const config = KGlobal::config();

    saveMainWindowSettings( config, "window" );
    m_recentScans->saveEntries( config );
    config->setGroup( "general" );
    config->writePathEntry( "comboHistory", m_combo->historyItems() );
    config->sync();

    return true;
}

inline void
MainWindow::configToolbars() //slot
{
    KEditToolbar dialog( factory(), this );
    dialog.showButtonApply( false );

    if( dialog.exec() )
    {
        createGUI( m_part );
        applyMainWindowSettings( kapp->config(), "window" );
    }
}

inline void
MainWindow::configKeys() //slot
{
    KKeyDialog::configure( actionCollection(), this );
}

inline void
MainWindow::slotScanDirectory()
{
    slotScanUrl( KDirSelectDialog::selectDirectory( m_part->url().path(), false, this ) );
}

inline void MainWindow::slotScanHomeDirectory() { slotScanPath( getenv( "HOME" ) ); }
inline void MainWindow::slotScanRootDirectory() { slotScanPath( "/" ); }
inline void MainWindow::slotUp()                { slotScanUrl( m_part->url().upURL() ); }

inline void
MainWindow::slotComboScan()
{
   const QString path = KShell::tildeExpand(m_combo->lineEdit()->text());
   if( slotScanPath( path ) )
      m_combo->addToHistory( path );
}

inline bool
MainWindow::slotScanPath( const QString &path )
{
   return slotScanUrl( KURL::fromPathOrURL( path ) );
}

/*inline */bool //FIXME you may have to move this to the header :(
MainWindow::slotScanUrl( const KURL &url )
{
   const KURL oldUrl = m_part->url();
   const bool b = m_part->openURL( url );

   if( b ) {
      m_histories->push( oldUrl );
      actionCollection()->action( "go_back" )->KAction::setEnabled( false ); } //FIXME

   return b;
}

inline void
MainWindow::slotAbortScan()
{
    if( m_part->closeURL() ) actionCollection()->action( "scan_stop" )->setEnabled( false );
}

inline void
MainWindow::scanStarted()
{
    stateChanged( "scan_started" );
    m_combo->clearFocus();
}

inline void
MainWindow::scanFailed()
{
    stateChanged( "scan_failed" );
    actionCollection()->action( "go_up" )->setText( i18n( "Up" ) );
    m_combo->lineEdit()->clear();
}

void
MainWindow::scanCompleted()
{
    KAction *goUp  = actionCollection()->action( "go_up" );
    const KURL url = m_part->url();

    stateChanged( "scan_complete" );

    m_combo->lineEdit()->setText( m_part->prettyURL() );

    if( url.path( 1 ) == "/" )
    {
        goUp->setEnabled( false );
        goUp->setText( i18n( "Up" ) );
    }
    else goUp->setText( i18n( "Up: %1" ).arg( url.upURL().path( 1 ) ) );

    m_recentScans->addURL( url ); //FIXME doesn't set the tick
}

inline void
MainWindow::urlAboutToChange()
{
   //called when part's URL is about to change internally
   //the part will then create the Map and emit completed()

   m_histories->push( m_part->url() );
}


/**********************************************
  SESSION MANAGEMENT
 **********************************************/

void
MainWindow::saveProperties( KConfig *config ) //virtual
{
   m_histories->save( config );
   config->writeEntry( "currentMap", m_part->url().path() );
}

void
MainWindow::readProperties( KConfig *config ) //virtual
{
   m_histories->restore( config );
   slotScanPath( config->readEntry( "currentMap", QString::null ) );
}

} //namespace Filelight

#include "mainWindow.moc"
