//
// C++ Implementation: filelightpart
//
// Description:
//
//
// Author: Max Howell <mxcl@methylblue.com>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "filelightpart.h"
#include "part/kradialmap.h"
#include "scanmanager.h"
#include "settingsdlg.h"
#include "settings.h"

#include <qapplication.h> //overrideCursor()
#include <qwidget.h> //ctor
#include <qstatusbar.h>

#include <kinstance.h>
#include <kstdaction.h>
#include <kaction.h>
#include <klocale.h>
#include <kcursor.h>
#include <kparts/genericfactory.h>
#include <konq_operations.h>


Settings Gsettings( "filelightrc" );   //global options struct


typedef KParts::GenericFactory<FilelightPart> FilelightPartFactory;
K_EXPORT_COMPONENT_FACTORY( libfilelight, FilelightPartFactory )



FilelightBrowserExtension::FilelightBrowserExtension( FilelightPart* parent, const char *name )
  : KParts::BrowserExtension( parent, name )
{}

FilelightBrowserExtension::~FilelightBrowserExtension()
{}



FilelightPart::FilelightPart( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const QStringList& )
  : ReadOnlyPart( parent, name )
  , m_ext( new FilelightBrowserExtension( this, "browser_extension" ) )
  , m_manager( new ScanManager( this, "scan_manager" ) )
  , m_map( new KRadialMap( parentWidget, widgetName ) )
{
    Gsettings.readSettings();

    ScanManager::readMounts();

    KStdAction::zoomIn(  m_map, SLOT( slotZoomIn() ), actionCollection() );
    KStdAction::zoomOut( m_map, SLOT( slotZoomOut() ), actionCollection() );
    //FIXME std action is good or not for KParts?
    //KStdAction::preferences( this, SLOT( showSettings() ), actionCollection(), "configure_filelight" );
    new KAction( i18n( "Configure Filelight..." ), "configure", 0,
                       this, SLOT( showSettings() ),
                       actionCollection(), "configure_filelight" );

    setInstance( FilelightPartFactory::instance() );
    setWidget( m_map );
    setXMLFile( "filelight_partui.rc" );

    connect( m_map, SIGNAL( created( const Directory * ) ), SIGNAL( completed() ) );
    connect( m_map, SIGNAL( created( const Directory * ) ), SLOT( scanFinished() ) );
    connect( m_manager, SIGNAL( started( const QString & ) ), SLOT( scanStarted( const QString & ) ) );

    connect( m_map, SIGNAL( activated( const KURL & ) ), SLOT( updateURL( const KURL & ) ) );

    connect( m_manager, SIGNAL( started( const QString & ) ), m_map, SLOT( invalidate() ) );

    connect( m_manager, SIGNAL( cached( const Directory * ) ), m_map, SLOT( createFromCache( const Directory * ) ) );
    connect( m_manager, SIGNAL( succeeded( const Directory * ) ), m_map, SLOT( create( const Directory * ) ) );
    connect( m_manager, SIGNAL( failed( const QString &, ScanManager::ErrorCode ) ), SLOT( scanFailed( const QString &, ScanManager::ErrorCode ) ) );
//    connect( m_manager, SIGNAL( aborted() ), this, SLOT( scanAborted() ) );

    connect( m_manager, SIGNAL( cacheInvalidated() ), m_map, SLOT( invalidate() ) );

    connect( m_map, SIGNAL( hoverUpdated( const QString & ) ),  SLOT( hoverUpdated( const QString & ) ) );

}


FilelightPart::~FilelightPart()
{}


#include <kaboutdata.h>

KAboutData* FilelightPart::createAboutData()
{
  //redundant, silly to do twice and wasteful?

  static const char *description = I18N_NOOP("Recursive graphical display of disk usage.");
  static const char *homepage    = "http://www.methylblue.com/filelight/";
  static const char *bugs        = "filelight@methylblue.com";
  static const char *copyright   = "(C) 2003 Max Howell";
  static const char *more        = I18N_NOOP( "Filelight is available as a KPart and stand-alone application" );

  KAboutData* aboutData = new KAboutData( "filelight", "Filelight", "0.7.0", description, KAboutData::License_GPL_V2, copyright, more, homepage, bugs );

  aboutData->addAuthor( "Max Howell", I18N_NOOP("Author"), "max.howell@methylblue.com", "http://www.methylblue.com/" );
  aboutData->addCredit( "Steffen Gerlach", I18N_NOOP("Original concept"), 0, "http://www.steffengerlach.de/" );
  aboutData->addCredit( "André Somers", I18N_NOOP("Internationalisation Support"), "a.t.somers@student.utwente.nl" );
  aboutData->addCredit( "Stephanie James", I18N_NOOP("\"Girlfriend Usability Testing\"") );
  aboutData->addCredit( "Marcel Meyer", I18N_NOOP("Testing, bug reports and suggestions") );

  return aboutData;
}


bool FilelightPart::openURL( const KURL &u )
{
    bool b;

    if( u == this->url() )
    {
        slotRescan();
        b = true;
    }
    else
        b = m_manager->start( u );

    if( b )
    {
        m_url = u;
        emit started( 0 );
    }

    return b;
}


bool FilelightPart::closeURL()
{
    m_manager->abort();
    return true;
}


void FilelightPart::updateURL( const KURL &u )
{
    //FIXME instead there must be a way to check if started and if not do this

    m_url = u;

    emit m_ext->openURLNotify();
    emit m_ext->setLocationBarURL( u.prettyURL() );
}


void FilelightPart::slotRescan()
{
  //**** this is far from ideal. You lose the whole cache! FIXME!
  //this disconnection to prevent histories from being "updated"
//  disconnect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_histories, SLOT( push( const KURL & ) ) );
    connect( m_map, SIGNAL( invalidated( const KURL & ) ), m_manager, SLOT( _start( const KURL & ) ) );
    m_manager->emptyCache(); //causes canvas to invalidate
    disconnect( m_map, SIGNAL( invalidated( const KURL & ) ), m_manager, SLOT( _start( const KURL & ) ) );
//  connect( m_canvas, SIGNAL( invalidated( const KURL & ) ), m_histories, SLOT( push( const KURL & ) ) );
}


void FilelightPart::scanFailed( const QString &path, ScanManager::ErrorCode err )
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
    s = i18n( "Unable to enter: %1\nYou do not have access rights to this location." ).arg( path );
    break;

  default:
    //FIXME UNTRANSLATED!
    s = i18n( "The scan was aborted." );
    return;
  }

  emit canceled( s );
}


void FilelightPart::scanStarted( const QString &path )
{
  QApplication::setOverrideCursor( KCursor::workingCursor() );
}


void FilelightPart::scanFinished()
{
    emit setWindowCaption( m_url.prettyURL() );
    QApplication::restoreOverrideCursor();
}



void FilelightPart::showSettings()
{
  SettingsDlg *dialog = new SettingsDlg( &Gsettings, widget(), "settings_dialog" );

  connect( dialog, SIGNAL( canvasIsDirty( int ) ), m_map, SLOT( refresh( int ) ) );
  connect( dialog, SIGNAL( mapIsInvalid() ), m_manager, SLOT( emptyCache() ) );

  dialog->show(); //deletes itself
}


void FilelightPart::hoverUpdated( const QString &fullpath )
{
    emit newHoverFilename( fullpath );
    // TODO: in case we are really running inside konqueror, tell konqueror to
    //       update its statusbar
    //       <mcamen@mcamen.de>
}


#include "filelightpart.moc"
