//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include "define.h"
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kurl.h>
#include "mainWindow.h"


static const KCmdLineOptions options[] =
{
    { "+[path]", I18N_NOOP( "Scan 'path'" ), 0 },
    { 0, 0, 0 }
};

static KAboutData about(
        APP_NAME, I18N_NOOP( APP_PRETTYNAME ), APP_VERSION,
        I18N_NOOP("Graphical disk-usage information"), KAboutData::License_GPL_V2,
        I18N_NOOP("(C )2006 Max Howell"), 0,
        "http://www.methylblue.com/filelight/", "filelight@methylblue.com" );


int main( int argc, char *argv[] )
{
    using Filelight::MainWindow;

    about.addAuthor( "Max Howell", I18N_NOOP("Author, maintainer"), "max.howell@methylblue.com", "http://www.methylblue.com/" );
    about.addAuthor( "Mike Diehl", I18N_NOOP("Documentation"), 0, 0 );
    about.addCredit( "Steffen Gerlach", I18N_NOOP("Inspiration"), 0, "http://www.steffengerlach.de/" );
    about.addCredit( "André Somers",   I18N_NOOP("Internationalization") );
    about.addCredit( "Stephanie James", I18N_NOOP("Testing") );
    about.addCredit( "Marcus Camen",    I18N_NOOP("Bravery in the face of unreadable code") );

    KCmdLineArgs::init( argc, argv, &about );
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    if (!app.isRestored()) {
        MainWindow *mw = new MainWindow();
        app.setMainWidget( mw );

        KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();
        if (args->count() > 0 ) mw->scan( args->url( 0 ));
        args->clear();

        mw->show();
    }
    else RESTORE( MainWindow );

    return app.exec();
}
