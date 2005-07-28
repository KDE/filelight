/*
* disks.cpp
*
* Copyright (c) 1998 Michael Kropfberger <michael.kropfberger@gmx.net>
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

#include "debug.h"
#include "disk.h"
#include <kglobal.h>
#include <klocale.h>
#include <kprocess.h>
#include <kprogress.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qregexp.h>

Disk::Disk()
   : QObject()
   , size( 0 ), used( 0 ), avail( 0 )
   , isMounted( false ), readingSysStdErrOut( false )
{
   sysProc = new KShellProcess();
   Q_CHECK_PTR( sysProc );
   connect( sysProc, SIGNAL(receivedStdout( KProcess*, char*, int )), SLOT(receivedSysStdErrOut( KProcess*, char*, int )) );
   connect( sysProc, SIGNAL(receivedStderr( KProcess*, char*, int )), SLOT(receivedSysStdErrOut( KProcess*, char*, int )) );
   insertChild( sysProc );
}

void
Disk::guessIconName()
{
   if( mountPoint().contains( "cdrom", false ) )       icon = "cdrom";
   else if( deviceName().contains( "cdrom", false ) )  icon = "cdrom";
   else if( mountPoint().contains( "writer", false ) ) icon = "cdwriter";
   else if( deviceName().contains( "writer", false ) ) icon = "cdwriter";
   else if( mountPoint().contains( "mo", false ) )     icon = "mo";
   else if( deviceName().contains( "mo", false ) )     icon = "mo";
   else if( deviceName().contains( "fd", false ) ) {
      if( deviceName().contains( "360", false ) )      icon = "5floppy";
      if( deviceName().contains( "1200", false ) )     icon = "5floppy";
      else
         icon = "3floppy";
   }
   else if( mountPoint().contains( "floppy", false ) ) icon = "3floppy";
   else if( mountPoint().contains( "zip", false ) )    icon = "zip";
   else if( fsType().contains( "nfs", false ) )        icon = "nfs";
   else
      icon = "hdd";

   icon += mounted() ? "_mount" : "_unmount";
}


/**
 * Starts a command on the underlying system via /bin/sh
 */
int
Disk::sysCall( const QString & command )
{
   if( readingSysStdErrOut || sysProc->isRunning() )
      return -1;

   sysStringErrOut = i18n( "Called: %1\n\n" ).arg( command ); // put the called command on ErrOut
   sysProc->clearArguments();
   ( *sysProc ) << command;
   if( !sysProc->start( KProcess::Block, KProcess::AllOutput ) )
      fatal() << i18n( "could not execute %1" ).arg( command.local8Bit().data() ) << endl;

   if( sysProc->exitStatus() != 0 )
      emit sysCallError( this, sysProc->exitStatus() );

   return ( sysProc->exitStatus() );
}

void
Disk::receivedSysStdErrOut( KProcess *, char *data, int len )
{
   sysStringErrOut += QString::fromLocal8Bit( data, len );
}

QString
Disk::realDeviceName() const
{
   const QFileInfo info( device );
   QString relativePath = info.fileName();

   if( info.isSymLink() ) {
      const QString link = info.readLink();
      if( link.startsWith( "/" ) )
         return link;
      relativePath = link;
   }

   return QDir( info.dirPath( true ) ).canonicalPath() + '/' + relativePath;
}

QString
Disk::realMountPoint() const
{
   return QDir( mount ).canonicalPath();
}

void
Disk::setUsedKB( int kb_used )
{
   used = kb_used;
   if ( size < (used + avail) ) {  //adjust kBAvail
      kdWarning() << "device " << device << ": kBAvail(" << avail << ")+*kBUsed(" << used << ") exceeds kBSize(" << size << ")" << endl;
      setFreeKB( size - used );
   }
}

void
Disk::setFreeKB( int kb_avail )
{
   avail = kb_avail;
   if ( size < (used + avail) ) {  //adjust kBUsed
      kdWarning() << "device " << device << ": *kBAvail(" << avail << ")+kBUsed(" << used << ") exceeds kBSize(" << size << ")" << endl;
      setUsedKB( size - avail );
   }
}

#include "disk.moc"
