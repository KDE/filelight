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

static KAboutData about( APP_NAME, I18N_NOOP( APP_PRETTYNAME ), VERSION,
                         I18N_NOOP( "Graphical disk-usage information" ), KAboutData::License_GPL_V2,
                         I18N_NOOP( "(C) 2003 Max Howell" ), "",
                         "http://www.methylblue.com/filelight/", "filelight@methylblue.com" );


int main( int argc, char *argv[] )
{
    using Filelight::MainWindow;

    about.addAuthor( "Max Howell", I18N_NOOP("Me"), "max.howell@methylblue.com", "http://www.methylblue.com/" );
    about.addAuthor( "Mike Diehl", I18N_NOOP("Handbook guru"), 0, 0 );

    //TODO complete this list!
    about.addCredit( "Steffen Gerlach", I18N_NOOP("Inspiration"), 0, "http://www.steffengerlach.de/" );
    about.addCredit( "André Somers",   I18N_NOOP("Internationalisation support") );
    about.addCredit( "Stephanie James", I18N_NOOP("\"Girlfriend usability-testing\"") );
    about.addCredit( "Marcus Camen",    I18N_NOOP("Bravery in the face of unreadable code (patches)") );

    about.addCredit( "Kevin Donnelly",     I18N_NOOP("Welsh Translation") );
    about.addCredit( "Marcel Meyer",       I18N_NOOP("German Translation, testing, bug reports and suggestions") );
    about.addCredit( "Michal Sulek",       I18N_NOOP("Slovak Translation") );
    about.addCredit( "Michal Kosmulski",   I18N_NOOP("Polish Translation") );
    about.addCredit( "Andy Teijelo Pérez",I18N_NOOP("Spanish Translation") );
    about.addCredit( "Virgile Gerecke",    I18N_NOOP("French Translation") );
    about.addCredit( "Nick Shafff",        I18N_NOOP("Russian Translation") );
    about.addCredit( "Rinse de Vries",     I18N_NOOP("Dutch Translation") );

    KCmdLineArgs::init( argc, argv, &about );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    KApplication app;

    if( !app.isRestored() )
    {
        MainWindow *mw = new MainWindow();
        app.setMainWidget( mw );

        KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();
        if( args->count() > 0 ) mw->scan( args->url( 0 ) );
        args->clear();

        mw->show();
    }
    else RESTORE( MainWindow );

    return app.exec();
}
