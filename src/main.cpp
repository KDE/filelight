/***************************************************************************
                          main.cpp  -  description
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
 
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kurl.h>
#include <kapp.h>

#include "define.h"
#include "filelight.h"


static KCmdLineOptions options[] =
{
  { "+[path]", I18N_NOOP( "Scan 'path'" ), 0 },
  { 0, 0, 0 }
};



int main(int argc, char *argv[])
{
/* 
 KAboutData ( const char *appName, const char *programName,
                const char *version, const char *shortDescription = 0,
                int licenseType = License_Unknown, const char *copyrightStatement = 0,
                const char *text = 0, const char *homePageAddress = 0,
                const char *bugsEmailAddress = "submit@bugs.kde.org" ) */

//**** add more detail to description
  static const char *description = I18N_NOOP("Recursive graphical display of disk usage.");
  static const char *homepage    = "http://www.methylblue.com/filelight/";
  static const char *bugs        = "filelight@methylblue.com";
                
  KAboutData aboutData( PACKAGE, I18N_NOOP( PRETTYNAME ),
    VERSION, description, KAboutData::License_GPL_V2, "(c) Max Howell",
    I18N_NOOP( "Original concept by Steffen Gerlach" ), homepage, bugs );
    
  aboutData.addAuthor( "Max Howell", I18N_NOOP("Author"), "max.howell@methylblue.com", "http://www.methylblue.com/" );
  aboutData.addCredit( "Steffen Gerlach", I18N_NOOP("Original concept"), 0, "http://www.steffengerlach.de/" );
  aboutData.addCredit( "André Somers", I18N_NOOP("Internationalisation Support"), "a.t.somers@student.utwente.nl" );



  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication a;
  if( a.isRestored() )
    RESTORE( Filelight )
  else
  {
    Filelight *t = new Filelight();
    a.setMainWidget( t );

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    
    //scan any command line arguments
    if( args->count() > 0 )
      t->slotScanUrl( args->url( 0 ) );

    args->clear();

    t->show();
  }

  return a.exec();
}
