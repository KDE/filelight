/*
* disklister.h
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


#ifndef DISKLISTER_H
#define DISKLISTER_H

#include "disk.h"
#include <kconfig.h>
#include <klocale.h>
#include <qglobal.h> // defines the os-type

#define DF_COMMAND    "df"

// be pessimistic: df -T only works under linux !??
#if defined(_OS_LINUX_)
#define DF_ARGS       "-kT"
#define NO_FS_TYPE    false
#else
#define DF_ARGS       "-k"
#define NO_FS_TYPE    true
#endif

#ifdef _OS_SOLARIS_
#define CACHEFSTAB "/etc/cachefstab"
#define FSTAB "/etc/vfstab"
#else
#define FSTAB "/etc/fstab"
#endif

#define SEPARATOR "|"


class DiskList : public QObject, public QPtrList<Disk>
{
   Q_OBJECT
public:
   DiskList( QObject *parent );

   int readFSTAB();
   int readDF();

   void deleteAllMountedAt( const QString &mountpoint );
   void setUpdatesDisabled( bool disable );

signals:
   void readDFDone();
   void criticallyFull( Disk *disk );

private slots:
   void receivedDFStdErrOut( KProcess *, char *data, int len );
   void dfDone();

private:
   void replaceDeviceEntry( Disk *disk );

   KProcess *dfProc;
   QString   dfStringErrOut;
   bool      readingDFStdErrOut;
   bool      updatesDisabled;
};

#endif
