/*
* disklist.cpp
*
* $Id$
*
* Copyright (c) 1999 Michael Kropfberger <michael.kropfberger@gmx.net>
* Copyright (c) 2004 Max Howell <max.howell@methylblue.com>
*
* Requires the Qt widget libraries, available at no cost at
* http://www.troll.no/
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.
*/

#include <config.h>
#include "debug.h"
#include "diskLister.h"
#include <kapplication.h>
#include <kprocess.h>
#include <math.h>
#include <qfile.h>
#include <stdlib.h>

#define BLANK ' '
#define DELIMITER '#'
#define FULL_PERCENT 95.0


DiskList::DiskList( QObject *parent )
   : QObject( parent )
   , QPtrList<Disk>()
{
   updatesDisabled = false;

   if( NO_FS_TYPE )
      debug() << "df gives no FS_TYPE" << endl;

   dfProc = new KProcess();
   Q_CHECK_PTR( dfProc );
   connect( dfProc, SIGNAL( receivedStdout( KProcess*, char*, int ) ), SLOT( receivedDFStdErrOut( KProcess *, char *, int ) ) );
   connect( dfProc, SIGNAL( processExited( KProcess* ) ), SLOT( dfDone() ) );

   readingDFStdErrOut = FALSE;
}

/**
 * Updated need to be disabled sometimes to avoid pulling the Disk out from the popupmenu handler
 */
void
DiskList::setUpdatesDisabled( bool disable )
{
   updatesDisabled = disable;
}

static QString
expandEscapes( const QString& s )
{
   QString rc;
   for ( unsigned int i = 0; i < s.length(); i++ ) {
      if ( s[ i ] == '\\' ) {
         i++;
         switch ( s[ i ] ) {
         case '\\':   // backslash '\'
            rc += '\\';
            break;
         case '0':  // octal 0nn
            rc += static_cast<char>( s.mid( i, 3 ).toInt( 0, 8 ) );
            i += 2;
            break;
         default:
            // give up and not process anything else because I'm too lazy
            // to implement other escapes
            rc += '\\';
            rc += s[ i ];
            break;
         }
      } else {
         rc += s[ i ];
      }
   }
   return rc;
}

/**
 * Tries to figure out the possibly mounted fs
 */

int
DiskList::readFSTAB()
{
   debug() << k_funcinfo << endl;

   if ( readingDFStdErrOut || dfProc->isRunning() )
      return -1;

   QFile f( FSTAB );
   if( f.open( IO_ReadOnly ) ) {
      QTextStream t ( &f );
      QString s;
      Disk *disk;

      while( ! t.eof() ) {
         s = t.readLine();
         s = s.simplifyWhiteSpace();
         if( !s.isEmpty() && s.find( DELIMITER ) != 0 ) {
            // not empty or commented out by '#'

            debug() << "GOT: [" << s << "]" << endl;

            disk = new Disk(); // Q_CHECK_PTR(disk);
            disk->setMounted( FALSE );
            disk->setDeviceName( expandEscapes( s.left( s.find( BLANK ) ) ) );
            s = s.remove( 0, s.find( BLANK ) + 1 );

            debug() << "    deviceName:    [" << disk->deviceName() << "]" << endl;

            #ifdef _OS_SOLARIS_
               //device to fsck
               s = s.remove( 0, s.find( BLANK ) + 1 );
            #endif

            disk->setMountPoint( expandEscapes( s.left( s.find( BLANK ) ) ) );
            s = s.remove( 0, s.find( BLANK ) + 1 );
            //debug() << "    MountPoint:    [" << disk->mountPoint() << "]" << endl;
            //debug() << "    Icon:          [" << disk->iconName() << "]" << endl;
            disk->setFsType( s.left( s.find( BLANK ) ) );
            s = s.remove( 0, s.find( BLANK ) + 1 );
            //debug() << "    FS-Type:       [" << disk->fsType() << "]" << endl;
            disk->setMountOptions( s.left( s.find( BLANK ) ) );
            s = s.remove( 0, s.find( BLANK ) + 1 );
            //debug() << "    Mount-Options: [" << disk->mountOptions() << "]" << endl;
            if( ( disk->deviceName() != "none" )
                  && ( disk->fsType() != "swap" )
                  && ( disk->mountPoint() != "/dev/swap" )
                  && ( disk->mountPoint() != "/dev/pts" )
                  && ( disk->mountPoint().find( "/proc" ) == -1 ) )
               replaceDeviceEntry( disk );
            else
               delete disk;

         } //if not empty
      } //while
      f.close();
   } //if f.open

   return 1;
}


/**
 * Is called, when the df-command writes on StdOut or StdErr
 */
void
DiskList::receivedDFStdErrOut( KProcess*, char *data, int len )
{
   debug() << k_funcinfo << endl;

   /* ATTENTION: StdERR no longer connected to this...
    * Do we really need StdErr?? on HP-UX there was eg. a line
    * df: /home_tu1/ijzerman/floppy: Stale NFS file handle
    * but this shouldn't cause a real problem
    */

   dfStringErrOut += QString::fromLatin1( data, len );
}

/**
 * Reads the df-commands results
 */
int
DiskList::readDF()
{
   debug() << k_funcinfo << endl;

   if( readingDFStdErrOut || dfProc->isRunning() )
      return -1;
   setenv( "LANG", "en_US", 1 );
   setenv( "LC_ALL", "en_US", 1 );
   setenv( "LC_MESSAGES", "en_US", 1 );
   setenv( "LC_TYPE", "en_US", 1 );
   setenv( "LANGUAGE", "en_US", 1 );

   dfStringErrOut = QString::null; // yet no data received

   dfProc->clearArguments();
   ( *dfProc ) << "env" << "LC_ALL=POSIX" << DF_COMMAND << DF_ARGS;
   if ( !dfProc->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
      fatal() << "Could not execute: " << DF_COMMAND << endl;

   return 1;
}


/**
 * Is called, when the df-command has finished
 */
void
DiskList::dfDone()
{
   debug() << k_funcinfo << endl;

   if( updatesDisabled )
      return ; //Don't touch the data for now..

   readingDFStdErrOut = TRUE;
   for ( Disk * disk = first(); disk != 0; disk = next() )
      disk->setMounted( FALSE );  // set all devs unmounted

   QTextStream t ( dfStringErrOut, IO_ReadOnly );
   QString s = t.readLine();
   if( s.isEmpty() || !s.startsWith( "Filesystem" ) )
      qFatal( "Error running df command... got [%s]", s.latin1() );

   while( !t.atEnd() ) {
      QString u, v;
      Disk *disk;
      s = t.readLine();
      s = s.simplifyWhiteSpace();
      if ( !s.isEmpty() ) {
         disk = new Disk();
         Q_CHECK_PTR( disk );

         if ( s.find( BLANK ) < 0 )       // devicename was too long, rest in next line
            if ( !t.eof() ) {       // just appends the next line
               v = t.readLine();
               s = s.append( v.latin1() );
               s = s.simplifyWhiteSpace();
               //debug() << "SPECIAL GOT: [" << s << "]" << endl;
            } //if silly linefeed

         //debug() << "EFFECTIVELY GOT " << s.length() << " chars: [" << s << "]" << endl;

         disk->setDeviceName( s.left( s.find( BLANK ) ) );
         s = s.remove( 0, s.find( BLANK ) + 1 );
         //debug() << "    DeviceName:    [" << disk->deviceName() << "]" << endl;

         if( NO_FS_TYPE ) {
            //debug() << "THERE IS NO FS_TYPE_FIELD!" << endl;
            disk->setFsType( "?" );
         } else {
            disk->setFsType( s.left( s.find( BLANK ) ) );
            s = s.remove( 0, s.find( BLANK ) + 1 );
         };
         //debug() << "    FS-Type:       [" << disk->fsType() << "]" << endl;
         //debug() << "    Icon:          [" << disk->iconName() << "]" << endl;

         u = s.left( s.find( BLANK ) );
         disk->setSizeKB( u.toInt() );
         s = s.remove( 0, s.find( BLANK ) + 1 );
         //debug() << "    Size:       [" << disk->kBSize() << "]" << endl;

         u = s.left( s.find( BLANK ) );
         disk->setUsedKB( u.toInt() );
         s = s.remove( 0, s.find( BLANK ) + 1 );
         //debug() << "    Used:       [" << disk->kBUsed() << "]" << endl;

         u = s.left( s.find( BLANK ) );
         disk->setFreeKB( u.toInt() );
         s = s.remove( 0, s.find( BLANK ) + 1 );
         //debug() << "    Avail:       [" << disk->kBAvail() << "]" << endl;


         s = s.remove( 0, s.find( BLANK ) + 1 );  // delete the capacity 94%
         disk->setMountPoint( s );
         //debug() << "    MountPoint:       [" << disk->mountPoint() << "]" << endl;

         if( ( disk->sizeKB() > 0 )
               && ( disk->deviceName() != "none" )
               && ( disk->fsType() != "swap" )
               && ( disk->mountPoint() != "/dev/swap" )
               && ( disk->mountPoint() != "/dev/pts" )
               && ( disk->mountPoint().find( "/proc" ) == -1 ) ) {
            disk->setMounted( TRUE );    // its now mounted (df lists only mounted)
            replaceDeviceEntry( disk );
         } else
            delete disk;

         disk->guessIconName();

      } //if not header
   } //while further lines available

   readingDFStdErrOut = FALSE;

   emit readDFDone();
}


void
DiskList::deleteAllMountedAt( const QString &mountpoint )
{
   debug() << k_funcinfo << endl;

   for( Disk * item = first(); item; ) {
      if( item->mountPoint() == mountpoint ) {
         debug() << "delete " << item->deviceName() << endl;
         remove( item );
         item = current();
      }
      else
         item = next();
   }
}

/**
 * Updates or creates a new Disk in the KDFList and TabListBox
 */
void
DiskList::replaceDeviceEntry( Disk *disk )
{
   //debug() << k_funcinfo << disk->deviceRealName() << " " << disk->realMountPoint() << endl;

   //
   // The 'disks' may already already contain the 'disk'. If it do
   // we will replace some data. Otherwise 'disk' will be added to the list
   //

   //
   // 1999-27-11 Espen Sand:
   // I can't get find() to work. The Disks::compareItems(..) is
   // never called.
   //
   //int pos=disks->find(disk);

   QString deviceRealName = disk->realDeviceName();
   QString realMountPoint = disk->realMountPoint();

   int pos = -1;
   for ( u_int i = 0; i < count(); i++ ) {
      Disk *item = at( i );
      int res = deviceRealName.compare( item->realDeviceName() );
      if ( res == 0 ) {
         res = realMountPoint.compare( item->realMountPoint() );
      }
      if ( res == 0 ) {
         pos = i;
         break;
      }
   }

   if ( ( pos == -1 ) && ( disk->mounted() ) )
      // no matching entry found for mounted disk
      if ( ( disk->fsType() == "?" ) || ( disk->fsType() == "cachefs" ) ) {
         //search for fitting cachefs-entry in static /etc/vfstab-data
         Disk * olddisk = first();
         while ( olddisk != 0 ) {
            int p;
            // cachefs deviceNames have no / behind the host-column
            // eg. /cache/cache/.cfs_mnt_points/srv:_home_jesus
            //                                      ^    ^
            QString odiskName = olddisk->deviceName();
            int ci = odiskName.find( ':' ); // goto host-column
            while ( ( ci = odiskName.find( '/', ci ) ) > 0 ) {
               odiskName.replace( ci, 1, "_" );
            } //while
            // check if there is something that is exactly the tail
            // eg. [srv:/tmp3] is exact tail of [/cache/.cfs_mnt_points/srv:_tmp3]
            if ( ( ( p = disk->deviceName().findRev( odiskName
                         , disk->deviceName().length() ) )
                   != -1 )
                  && ( p + odiskName.length()
                       == disk->deviceName().length() ) ) {
               pos = at(); //store the actual position
               disk->setDeviceName( olddisk->deviceName() );
               olddisk = 0;
            } else
               olddisk = next();
         } // while
      } // if fsType == "?" or "cachefs"


#ifdef NO_FS_TYPE
   if ( pos != -1 )
   {
      Disk * olddisk = at( pos );
      if ( olddisk )
         disk->setFsType( olddisk->fsType() );
   }
#endif

   if ( pos != -1 ) {  // replace
      Disk * olddisk = at( pos );

//       if ( ( -1 != olddisk->mountOptions().find( "user" ) ) &&
//             ( -1 == disk->mountOptions().find( "user" ) ) ) {
//          // add "user" option to new Disk
//          QString s = disk->mountOptions();
//          if ( s.length() > 0 )
//             s.append( "," );
//          s.append( "user" );
//          disk->setMountOptions( s );
//       }
//       disk->setMountCommand( olddisk->mountCommand() );
//       disk->setUmountCommand( olddisk->umountCommand() );

      // Same device name, but maybe one is a symlink and the other is its target
      // Keep the shorter one then, /dev/hda1 looks better than /dev/ide/host0/bus0/target0/lun0/part1
      if ( disk->deviceName().length() > olddisk->deviceName().length() )
         disk->setDeviceName( olddisk->deviceName() );

      //FStab after an older DF ... needed for critFull
      //so the DF-KBUsed survive a FStab lookup...
      //but also an unmounted disk may then have a kbused set...
      if ( ( olddisk->mounted() ) && ( !disk->mounted() ) ) {
         disk->setSizeKB( olddisk->sizeKB() );
         disk->setUsedKB( olddisk->usedKB() );
         disk->setFreeKB( olddisk->freeKB() );
      }
      if ( ( olddisk->percentFull() != -1 ) &&
            ( olddisk->percentFull() < FULL_PERCENT ) &&
            ( disk->percentFull() >= FULL_PERCENT ) ) {
         debug() << "Device " << disk->deviceName()
         << " is critFull! " << olddisk->percentFull()
         << "--" << disk->percentFull() << endl;
         emit criticallyFull( disk );
      }
      remove( pos ); // really deletes old one
      insert( pos, disk );
   }
   else
      append( disk );
}

#include "diskLister.moc"
