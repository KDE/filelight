/***************************************************************************
                          scanmanager.cpp  -  description
                             -------------------
    begin                : Tue Oct 21 2003
    copyright            : (C) 2003 by Max Howell
    email                : max.howell@methylblue.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qapplication.h> //postEvent()
#include <qstringlist.h>  //start()
#include <kdebug.h>
#include <kurl.h>
#include <string.h>       //strdup()
 
//scan()
#include <dirent.h>
#include <sys/stat.h> //lstat()
#include <unistd.h>   //access()

//readMounts()
#include <fstab.h>
#include <mntent.h>
#include <sys/statfs.h>

#include "scanmanager.h"
#include "filetree.h"
#include "settings.h"



bool ScanThread::bAbort;
unsigned int ScanThread::fileCounter;
QStringList ScanManager::localMounts, ScanManager::remoteMounts;

extern Settings Gsettings;



ScanManager::ScanManager( QObject *parent, const char *name )
 : QObject( parent, name ), m_thread( new ScanThread )
{ }

ScanManager::~ScanManager()
{
  kdDebug() << "~ScanManager\n";

  if( m_thread->running() )
  {
    kdDebug() << "Stopping ScanThread..\n";
    ScanThread::abort();
    m_thread->wait();
  }
  
  delete m_thread;
}


void ScanManager::abort()
{
  m_thread->abort();
  emit aborted();
}


void ScanManager::start( const KURL &url, bool force )
{
  ScanManager::ScanError err = ScanManager::NoError;
  QString path;
  
  //**** need to add initial / if not present frankly
  //     as user might be confused if it isn't working, especially as we only accept absolute paths

  if( url.protocol() != "file" )
    err = ScanManager::InvalidProtocol;
  else if( !url.isValid() )
    err = ScanManager::InvalidUrl;
  else
  {
    struct stat statbuf;

    if( stat( url.path(), &statbuf ) == 0 )
    {
      if( !S_ISDIR( statbuf.st_mode ) )
        err = ScanManager::NotDirectory;
    }
    else
      err = ScanManager::UnableToStat;
  }
    
  if( err == ScanManager::NoError )
  {
    //**** maybe don't pass const instead of this mess?
    KURL tmp( url );
    tmp.cleanPath();
    QString path( tmp.path( 1 ) );
    startPrivate( path, force );
  }
  else
  {
    kdDebug() << err << endl;
    emit failed( url.path( 1 ) );
  }
}


void ScanManager::startPrivate( const QString &path, bool force )
{
  kdDebug() << "Scan requested for: " << path << "\n";

  if( m_thread->running() )
  {
    //shouldn't happen, but lets prevent mega-disasters just in case eh?
    kdWarning() << "Filelight attempted to run 2 scans concurrently!\n";
    //**** emit scanFailed with specific error message
    return;
  }


  Chain<Directory> *list = new Chain<Directory>;
  
  if( !force )
  {

  /* CHECK CACHE
   *   user wants: /usr/local/
   *   cached:     /usr/
   *
   *   user wants: /usr/
   *   cached:     /usr/local/, /usr/include/
   */

  for( Iterator<Directory> it = cache.iterator(); it != cache.end(); ++it )
  {
    QString cachePath( (*it)->name() );

    if( path.startsWith( cachePath ) ) //then whole tree already scanned
    {
      //find a pointer to the requested branch

      kdDebug() << "1Cache-hit: " << cachePath << endl;
      
      QStringList slist = QStringList::split( "/", path.mid( cachePath.length() ) );
      Directory *d = *it;
      Iterator<File> jt;

      while( !slist.isEmpty() && d != NULL ) //if NULL we have got lost so abort!!
      {
        jt = d->iterator();

        const Link<File> *end = d->end();
        QString s = slist.first() + "/";

        for( d = NULL; jt != end; ++jt )
          if( s == (*jt)->name() )
          {
            d = (Directory *)*jt;
            break;
          }

        slist.pop_front();
      }

      if( d != NULL )
      {
        //we found a completed tree, thus no need to scan
        kdDebug() << "Found cached tree\n";
        emit cached( d );
        return;
      }
      else
      {
        //something went wrong, we couldn't find the directory we were expecting
        kdError() << "Didn't find " << path << " in the cache!\n";
        delete it.remove(); //safest to get rid of it
        break; //do a full scan
      }
    }
    else if( cachePath.startsWith( path ) ) //then part of the requested tree is already scanned
    {
      kdDebug() << "2Cache-hit: " << cachePath << endl;
      it.transferTo( *list );
    }
  }
  }

  //add empty directories for any mount points that are in the path
  //**** empty directories is not ideal as adds to fileCount incorrectly
  if( !Gsettings.skipList.isEmpty() )
  {
    QStringList slist( Gsettings.skipList );
    if( !Gsettings.scanAcrossMounts )
      slist += localMounts;
    if( !Gsettings.scanRemoteMounts )
      slist += remoteMounts;

    for( QStringList::iterator it = slist.begin(); it != slist.end(); ++it )
      if( (*it).startsWith( path ) )
        list->append( new Directory( strdup( path ) ) );
  }
    
  //start separate thread, control is immediately returned
  ScanThread::fileCounter = 0;
  ScanThread::bAbort      = false;
  m_thread->init( path, this, list );

  //tell other parts of Filelight that a scan has started
  //** don't have race condition, do emit the signal first!!
  emit started( path );

  m_thread->start();
}


void ScanManager::customEvent( QCustomEvent * e )
{
  switch( e->type() ) {
  case 65433: //scan succeeded
    {
      Directory *tree = static_cast<ScanCompleteEvent*>(e)->tree();

      cache.append( tree );
      //**** very important to sanity check the cache now
      emit succeeded( tree );
    }

    break;

  case 65434: //scan failed //**** implement error messages into this

    emit failed( static_cast<ScanFailedEvent*>(e)->path() );

  default: break;
  }
}


bool ScanManager::readMounts()
{
  #define INFO_PARTITIONS "/proc/partitions"
  #define INFO_MOUNTED_PARTITIONS "/etc/mtab" /* on Linux... */

  //**** SHAMBLES
  //  ** mtab should have priority as mount points don't have to follow fstab
  //  ** no removable media detection
  //  ** no updates if mounts change
  //  ** you want a KDE extension that handles this for you really

  struct fstab *fstab_ent;
  struct mntent *mnt_ent;
  FILE *fp;
  QString str;

  if( setfsent() == 0 || !( fp = setmntent( INFO_MOUNTED_PARTITIONS, "r" ) ) )
    return false;

#define FS_NAME   fstab_ent->fs_spec    // device-name
#define FS_FILE   fstab_ent->fs_file    // mount-point
#define FS_TYPE   fstab_ent->fs_vfstype // fs-type
#define FS_MNTOPS fstab_ent->fs_mntops  // mount-options

  QStringList remoteFsTypes;
  remoteFsTypes << "smbfs" << MNTTYPE_NFS;

  while( (fstab_ent = getfsent()) != NULL )
  {
    str = QString( FS_FILE );
    if( str == "/" ) continue;
    str += "/";

    if( remoteFsTypes.contains( FS_TYPE ) )
      remoteMounts.append( str ); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

    else
      localMounts.append( str ); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

    kdDebug( 0 ) << "FSTAB: " << FS_TYPE << "\n";
  }

  endfsent();  /* close fstab.. */

#undef  FS_NAME
#undef  FS_FILE
#undef  FS_TYPE
#undef  FS_MNTOPS

#define FS_NAME   mnt_ent->mnt_fsname // device-name
#define FS_FILE   mnt_ent->mnt_dir    // mount-point
#define FS_TYPE   mnt_ent->mnt_type	  // fs-type
#define FS_MNTOPS mnt_ent->mnt_opts   // mount-options

  //scan mtab, **** mtab should take priority, but currently it isn't

  while( ( mnt_ent = getmntent( fp ) ) != NULL )
  {
    bool b = false;

    str = QString( FS_FILE );
    if( str == "/" ) continue;
    str += "/";

    if( remoteFsTypes.contains( FS_TYPE ) )
      if( b = !remoteMounts.contains( str ) )
        remoteMounts.append( str ); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

    else
      if( b = !localMounts.contains( str ) )
        localMounts.append( str ); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

    if( b )
      kdDebug( 0 ) << "MTAB: " << FS_TYPE << "\n";
  }

  endmntent( fp ); /* close mtab.. */


  return true;
}



//some GNU systems don't support big files for some reason
#ifndef __USE_LARGEFILE64 //see dirent.h
 #define DIRENT dirent
 #define SCANDIR scandir
 #define STATSTRUCT stat
 #define LSTAT lstat
#else
 #define DIRENT dirent64
 #define SCANDIR scandir64
 #define STATSTRUCT stat64
 #define LSTAT lstat64
#endif

#ifndef NULL
#define NULL 0
#endif


void ScanThread::run()
{
  Directory *tree = scan( m_path, m_path );

  //delete cache list
  //**** on aborts you should try to salvage the contents below
  delete m_trees; //shouldn't be anything valuable left in here, if there is, something is broken
  
  if( bAbort ) //scan was cancelled
  {
    kdDebug() << "Scan succesfully aborted\n";
    delete tree;
    tree = NULL;
  }


  QCustomEvent *e;

  if( tree == NULL )
    e = new ScanFailedEvent( m_path );
  else
    e = new ScanCompleteEvent( tree );

  QApplication::postEvent( m_parent, e );  //Qt will delete it for us
}



static int selector( struct DIRENT const *ent )
{
  if( strcmp( ent->d_name, "." ) == 0 || strcmp( ent->d_name, ".." ) == 0 )
    return 0;

  return 1;
}


Directory *ScanThread::scan( const QString &path, const QString &dirname )
{
  //stop scanning if we've been asked too
  //**** doing here is efficient, but can cause scans to have delayed stopping
  if( bAbort ) return NULL;
  
  if( access( path, R_OK ) != 0 )
  {
    kdWarning( 0 ) << "No access granted: " << path << "\n";
    return NULL;
  }

  Directory *cwd = new Directory( strdup( dirname ) );
  struct DIRENT **eps;
  int n = SCANDIR( path, &eps, selector, /*alphasort*/ NULL );

  if( n >= 0 ) {

    struct STATSTRUCT statbuf;

    //loop over array of dirents
    for( int cnt = 0; cnt < n; ++cnt ) {

      QString new_path = path + eps[cnt]->d_name;

      //get file information
      if( LSTAT( new_path, &statbuf ) != 0 )
        continue; //stat failed! **** you need to handle this better

      if( S_ISLNK(  statbuf.st_mode ) ||
          S_ISCHR(  statbuf.st_mode ) ||
          S_ISBLK(  statbuf.st_mode ) ||
          S_ISFIFO( statbuf.st_mode ) ||
          S_ISSOCK( statbuf.st_mode )
        )
      {
        continue;
      }

      if( S_ISREG( statbuf.st_mode ) )  //file
        cwd->append( strdup( eps[cnt]->d_name ), statbuf.st_size / 1024 ); //using units of kB as 32bit max is 4GB and 64bit ints are expensive

      if( S_ISDIR( statbuf.st_mode ) )  //directory
      {
        Directory *d = NULL;
        QString new_dirname( eps[cnt]->d_name );
        new_dirname += "/";
        new_path    += "/";

        //check to see if we've scanned this section already

        for( Iterator<Directory> it = m_trees->iterator(); it != m_trees->end(); ++it )
        {
          QString scanned_path( (*it)->name() );
          
          if( new_path == scanned_path )
          {
            kdDebug() << "Tree pre-completed: " << scanned_path << "\n";
            d = it.remove();
            fileCounter += d->fileCount();
            //**** ideally don't have this redundant extra somehow
            cwd->append( d, strdup( new_dirname ) );
          }
        }

        if( d == NULL ) //then scan
        {
          d = scan( new_path, new_dirname );
         if( d != NULL )
          cwd->append( d );
        }
      }

      ++fileCounter;
    }
  }
  else
    kdError( 0 ) << "No files found: " << path << "\n";

  //**** could just return a Chain<File*> and then reinstate const members of Directory
  return cwd; //might be empty
}

#include "scanmanager.moc"
