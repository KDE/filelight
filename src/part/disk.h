/*
* disk.h
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
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#ifndef __DISK_H__
#define __DISK_H__

#include <kio/global.h>
#include <qobject.h>

class KProcess;

class Disk : public QObject
{
Q_OBJECT
public:
   Disk();

   QString lastSysError() const { return sysStringErrOut; }

   /// The real device (in case deviceName() is a symlink)
   QString realMountPoint() const;
   /// The real device (in case deviceName() is a symlink)
   QString realDeviceName() const;

   QString deviceName() const { return device; }
   QString mountPoint() const { return mount; }
   QString prettyKBSize() const { return KIO::convertSizeFromKB( size ); }
   QString prettyKBUsed() const { return KIO::convertSizeFromKB( used ); }
   QString prettyKBFree() const { return KIO::convertSizeFromKB( avail ); }
   QString fsType() const { return type; }
   QString iconName() const { return icon; }
   float percentFull() const { return size == 0 ? -1 : 100 - ((float)avail / (float)size) * 100; }
   bool mounted() const { return isMounted; };
   int usedKB() const { return used; }
   int freeKB() const { return avail; }
   int sizeKB() const { return size; }

signals:
   void sysCallError( Disk *disk, int err_no );

public:
   void setMounted( bool b) { isMounted = b; }
   void setDeviceName( const QString &s ) { device = s; }
   void setMountPoint( const QString &s ) { mount = s; }
   void setMountOptions( const QString &s ) { options = s; }
   void setFsType( const QString &s ) { type = s; }
   void setSizeKB( int kb_size ) { size = kb_size; }
   void setUsedKB( int kb_used );
   void setFreeKB( int kb_avail );

  void guessIconName();

private slots:
   void receivedSysStdErrOut( KProcess*, char *data, int len );

private:
   int sysCall( const QString & command );

   class KShellProcess *sysProc;
   QString sysStringErrOut;

   QString device;
   QString type;
   QString mount;
   QString options;
   QString icon;

   int size;
   int used;
   int avail; //NOTE used+avail != size (clustersize!)

   bool isMounted;
   bool readingSysStdErrOut;
};

#endif
