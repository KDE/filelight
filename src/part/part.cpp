//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include "Config.h"
#include "debug.h"
#include "define.h"
#include "fileTree.h"
#include "part.h"
#include "progressBox.h"
#include "radialMap/widget.h"
#include "scan.h"
#include "settingsDialog.h"
#include "summaryWidget.h"

#include <kaboutdata.h>   //::createAboutData()
#include <kaction.h>
#include <klocale.h>
#include <kmessagebox.h>  //::start()
//#include <konq_operations.h>
#include <kparts/genericfactory.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <qfile.h>        //encodeName()
#include <qtimer.h>       //postInit() hack
#include <qvbox.h>
#include <unistd.h>       //access()


namespace Filelight {


typedef KParts::GenericFactory<Filelight::Part> Factory;
K_EXPORT_COMPONENT_FACTORY( libfilelight, Filelight::Factory )


BrowserExtension::BrowserExtension( Part *parent, const char *name )
  : KParts::BrowserExtension( parent, name )
{}


Part::Part( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const QStringList& )
        : ReadOnlyPart( parent, name )
        , m_ext( new BrowserExtension( this ) )
        , m_statusbar( new StatusBarExtension( this ) )
        , m_map( 0 )
        , m_manager( new ScanManager( this ) )
        , m_started( false )
{
    QPixmap::setDefaultOptimization( QPixmap::BestOptim );

    Config::read();

    setInstance( Factory::instance() );
    setWidget( new QVBox( parentWidget, widgetName ) );
    setXMLFile( "filelight_partui.rc" );

    m_map = new RadialMap::Widget( widget() );
    m_map->hide();

    KStdAction::zoomIn( m_map, SLOT(zoomIn()), actionCollection() );
    KStdAction::zoomOut( m_map, SLOT(zoomOut()), actionCollection() );
    KStdAction::preferences( this, SLOT(configFilelight()), actionCollection(), "configure_filelight" )->setText( i18n( "Configure Filelight..." ) );

    connect( m_map, SIGNAL(created( const Directory* )), SIGNAL(completed()) );
    connect( m_map, SIGNAL(created( const Directory* )), SLOT(mapChanged( const Directory* )) );
    connect( m_map, SIGNAL(activated( const KURL& )), SLOT(updateURL( const KURL& )) );

    // TODO make better system
    connect( m_map, SIGNAL(giveMeTreeFor( const KURL& )), SLOT(updateURL( const KURL& )) );
    connect( m_map, SIGNAL(giveMeTreeFor( const KURL& )), SLOT(openURL( const KURL& )) );

    connect( m_manager, SIGNAL(completed( Directory* )), SLOT(scanCompleted( Directory* )) );
    connect( m_manager, SIGNAL(aboutToEmptyCache()), m_map, SLOT(invalidate()) );

    QTimer::singleShot( 0, this, SLOT(postInit()) );
}

void
Part::postInit()
{
   if( m_url.isEmpty() ) //if url is not empty openURL() has been called immediately after ctor, which happens
   {
      QWidget *summary = new SummaryWidget( widget(), "summaryWidget" );
      connect( summary, SIGNAL(activated( const KURL& )), SLOT(openURL( const KURL& )) );
      summary->show();

      //FIXME KXMLGUI is b0rked, it should allow us to set this
      //BEFORE createGUI is called but it doesn't
      stateChanged( "scan_failed" );
   }
}

bool
Part::openURL( const KURL &u )
{
   //we don't want to be using the summary screen anymore
   delete widget()->child( "summaryWidget" );
   m_map->show();

   //TODO everyone hates dialogs, instead render the text in big fonts on the Map
   //TODO should have an empty KURL until scan is confirmed successful
   //TODO probably should set caption to QString::null while map is unusable

   #define KMSG( s ) KMessageBox::information( widget(), s )

   KURL url = u;
   url.cleanPath( true );
   const QString path = url.path( 1 );
   const QCString path8bit = QFile::encodeName( path );
   const bool isLocal = url.protocol() == "file";

   if( url.isEmpty() )
   {
      //do nothing, chances are the user accidently pressed ENTER
   }
   else if( !url.isValid() )
   {
      KMSG( i18n( "The entered URL cannot be parsed; it is invalid." ) );
   }
   else if( path[0] != '/' )
   {
      KMSG( i18n( "Filelight only accepts absolute paths, eg. /%1" ).arg( path ) );
   }
   else if( isLocal && access( path8bit, F_OK ) != 0 ) //stat( path, &statbuf ) == 0
   {
      KMSG( i18n( "Directory not found: %1" ).arg( path ) );
   }
   else if( isLocal && access( path8bit, R_OK | X_OK ) != 0 )
   {
      KMSG( i18n( "Unable to enter: %1\nYou do not have access rights to this location." ).arg( path ) );
   }
   else
   {
      if( url == m_url )
         m_manager->emptyCache(); //same as rescan()

      return start( url );
   }

   return false;
}

bool
Part::closeURL()
{
   if( m_manager->abort() )
      statusBar()->message( i18n( "Aborting Scan..." ) );

   m_url = KURL();

   return true;
}

void
Part::updateURL( const KURL &u )
{
   //the map has changed internally, update the interface to reflect this
   emit m_ext->openURLNotify(); //must be done first
   emit m_ext->setLocationBarURL( u.prettyURL() );

   //do this last, or it breaks Konqi location bar
   m_url = u;
}

void
Part::configFilelight()
{
    QWidget *dialog = new SettingsDialog( widget(), "settings_dialog" );

    connect( dialog, SIGNAL(canvasIsDirty( int )), m_map, SLOT(refresh( int )) );
    connect( dialog, SIGNAL(mapIsInvalid()), m_manager, SLOT(emptyCache()) );

    dialog->show(); //deletes itself
}

KAboutData*
Part::createAboutData()
{
    return new KAboutData( APP_NAME, I18N_NOOP( APP_PRETTYNAME ), APP_VERSION );
}

bool
Part::start( const KURL &url )
{
   if( !m_started ) {
      m_statusbar->addStatusBarItem( new ProgressBox( statusBar(), this ), 0, true );
      connect( m_map, SIGNAL(mouseHover( const QString& )), statusBar(), SLOT(message( const QString& )) );
      connect( m_map, SIGNAL(created( const Directory* )), statusBar(), SLOT(clear()) );
      m_started = true;
   }

   if( m_manager->start( url ) ) {
      m_url = url;

      const QString s = i18n( "Scanning: %1" ).arg( prettyURL() );
      stateChanged( "scan_started" );
      emit started( 0 ); //as a Part, we have to do this
      emit setWindowCaption( s );
      statusBar()->message( s );
      m_map->invalidate(); //to maintain ui consistency

      return true;
   }

   return false;
}

void
Part::rescan()
{
   //FIXME we have to empty the cache because otherwise rescan picks up the old tree..
   m_manager->emptyCache(); //causes canvas to invalidate
   start( m_url );
}

void
Part::scanCompleted( Directory *tree )
{
   if( tree ) {
      statusBar()->message( i18n( "Scan completed, generating map..." ) );

      m_map->create( tree );

      //do after creating map
      stateChanged( "scan_complete" );
   }
   else {
      stateChanged( "scan_failed" );
      emit canceled( i18n( "Scan failed: %1" ).arg( prettyURL() ) );
      emit setWindowCaption( QString::null );

      statusBar()->clear();
//      QTimer::singleShot( 2000, statusBar(), SLOT(clear()) );

      m_url = KURL();
   }
}

void
Part::mapChanged( const Directory *tree )
{
   //IMPORTANT -> m_url has already been set

   emit setWindowCaption( prettyURL() );

   ProgressBox *progress = static_cast<ProgressBox *>(statusBar()->child( "ProgressBox" ));

   if( progress )
      progress->setText( tree->children() );
}

} //namespace Filelight

#include "part.moc"
