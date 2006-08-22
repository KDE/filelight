//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include "Config.h"
#include "debug.h"
#include <dirent.h>
#include "fileTree.h"
#include <fstab.h>
#include "localLister.h"
#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif
#include <qapplication.h> //postEvent()
#include <qfile.h>
#include "scan.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace Filelight
{
   QStringList LocalLister::s_remoteMounts;
   QStringList LocalLister::s_localMounts;

   LocalLister::LocalLister( const QString &path, Chain<Directory> *cachedTrees, QObject *parent )
      : QThread()
      , m_path( path )
      , m_trees( cachedTrees )
      , m_parent( parent )
   {
      //add empty directories for any mount points that are in the path
      //TODO empty directories is not ideal as adds to fileCount incorrectly

      QStringList list( Config::skipList );
      if( !Config::scanAcrossMounts ) list += s_localMounts;
      if( !Config::scanRemoteMounts ) list += s_remoteMounts;

      for( QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it )
         if( (*it).startsWith( path ) )
            //prevent scanning of these directories
            m_trees->append( new Directory( (*it).local8Bit() ) );

      start();
   }

   void
   LocalLister::run()
   {
      //recursively scan the requested path
      const QCString path = QFile::encodeName( m_path );
      Directory *tree = scan( path, path );

      //delete the list of trees useful for this scan,
      //in a sucessful scan the contents would now be transfered to 'tree'
      delete m_trees;

      if( ScanManager::s_abort ) //scan was cancelled
      {
         debug() << "Scan succesfully aborted\n";
         delete tree;
         tree = 0;
      }

      QCustomEvent *e = new QCustomEvent( 1000 );
      e->setData( tree );
      QApplication::postEvent( m_parent, e );
   }

   // from system.h in GNU coreutils package
   /* Extract or fake data from a `struct stat'.
   ST_BLKSIZE: Preferred I/O blocksize for the file, in bytes.
   ST_NBLOCKS: Number of blocks in the file, including indirect blocks.
   ST_NBLOCKSIZE: Size of blocks used when calculating ST_NBLOCKS.  */
   #ifndef HAVE_STRUCT_STAT_ST_BLOCKS
      #define ST_BLKSIZE(statbuf) DEV_BSIZE
      #if defined _POSIX_SOURCE || !defined BSIZE /* fileblocks.c uses BSIZE.  */
         #define ST_NBLOCKS(statbuf) ((statbuf).st_size / ST_NBLOCKSIZE + ((statbuf).st_size % ST_NBLOCKSIZE != 0))
      #else /* !_POSIX_SOURCE && BSIZE */
         #define ST_NBLOCKS(statbuf) (S_ISREG ((statbuf).st_mode) || S_ISDIR ((statbuf).st_mode) ? st_blocks ((statbuf).st_size) : 0)
      #endif /* !_POSIX_SOURCE && BSIZE */
   #else /* HAVE_STRUCT_STAT_ST_BLOCKS */
      /* Some systems, like Sequents, return st_blksize of 0 on pipes.
         Also, when running `rsh hpux11-system cat any-file', cat would
         determine that the output stream had an st_blksize of 2147421096.
         So here we arbitrarily limit the `optimal' block size to 4MB.
         If anyone knows of a system for which the legitimate value for
         st_blksize can exceed 4MB, please report it as a bug in this code.  */
      #define ST_BLKSIZE(statbuf) ((0 < (statbuf).st_blksize && (statbuf).st_blksize <= (1 << 22)) /* 4MiB */ ? (statbuf).st_blksize : DEV_BSIZE)
      #if defined hpux || defined __hpux__ || defined __hpux
         /* HP-UX counts st_blocks in 1024-byte units.
            This loses when mixing HP-UX and BSD filesystems with NFS.  */
         #define ST_NBLOCKSIZE 1024
      #else /* !hpux */
         #if defined _AIX && defined _I386
            /* AIX PS/2 counts st_blocks in 4K units.  */
            #define ST_NBLOCKSIZE (4 * 1024)
         #else /* not AIX PS/2 */
            #if defined _CRAY
               #define ST_NBLOCKS(statbuf) (S_ISREG ((statbuf).st_mode) || S_ISDIR ((statbuf).st_mode) ? (statbuf).st_blocks * ST_BLKSIZE(statbuf)/ST_NBLOCKSIZE : 0)
            #endif /* _CRAY */
         #endif /* not AIX PS/2 */
      #endif /* !hpux */
   #endif /* HAVE_STRUCT_STAT_ST_BLOCKS */

   #ifndef ST_NBLOCKS
      #define ST_NBLOCKS(statbuf) ((statbuf).st_blocks)
   #endif

   #ifndef ST_NBLOCKSIZE
      #define ST_NBLOCKSIZE 512
   #endif

//some GNU systems don't support big files for some reason
#ifdef __USE_LARGEFILE64 //see dirent.h
 #define dirent dirent64
 #define scandir scandir64
 #define stat stat64
 #define statstruct stat64
 #define lstat lstat64
 #define readdir readdir64
#endif

#ifndef NULL
#define NULL 0
#endif


   #include <errno.h>
   static void
   outputError( QCString path )
   {
      ///show error message that stat or opendir may give

      #define out( s ) error() << s ": " << path << endl; break

      switch( errno ) {
      case EACCES:
         out( "Inadequate access permisions" );
      case EMFILE:
         out( "Too many file descriptors in use by Filelight" );
      case ENFILE:
         out( "Too many files are currently open in the system" );
      case ENOENT:
         out( "A component of the path does not exist, or the path is an empty string" );
      case ENOMEM:
         out( "Insufficient memory to complete the operation" );
      case ENOTDIR:
         out( "A component of the path is not a directory" );
      case EBADF:
         out( "Bad file descriptor" );
      case EFAULT:
         out( "Bad address" );
      case ELOOP: //NOTE shouldn't ever happen
         out( "Too many symbolic links encountered while traversing the path" );
      case ENAMETOOLONG:
         out( "File name too long" );
      }

      #undef out
   }

   Directory*
   LocalLister::scan( const QCString &path, const QCString &dirname )
   {
      Directory *cwd = new Directory( dirname );
      DIR       *dir = opendir( path );

      if( !dir ) {
         outputError( path );
         return cwd;
      }

      struct stat statbuf;
      dirent *ent;
      while ((ent = readdir( dir )))
      {
         if( ScanManager::s_abort )
            return cwd;

         if( qstrcmp( ent->d_name, "." ) == 0 || qstrcmp( ent->d_name, ".." ) == 0 )
            continue;

         QCString new_path = path; new_path += ent->d_name;

         //get file information
         if( lstat( new_path, &statbuf ) == -1 ) {
            outputError( new_path );
            continue;
         }

         if( S_ISLNK( statbuf.st_mode ) ||
            S_ISCHR(  statbuf.st_mode ) ||
            S_ISBLK(  statbuf.st_mode ) ||
            S_ISFIFO( statbuf.st_mode ) ||
            S_ISSOCK( statbuf.st_mode ) )
         {
            continue;
         }

         if( S_ISREG( statbuf.st_mode ) ) //file
            //using units of KiB as 32bit max is 4GiB and 64bit ints are expensive
            cwd->append( ent->d_name, (ST_NBLOCKS( statbuf ) * ST_NBLOCKSIZE) / 1024 );

         else if( S_ISDIR( statbuf.st_mode ) )  //directory
         {
            Directory *d = 0;
            QCString new_dirname = ent->d_name;
            new_dirname += '/';
            new_path    += '/';

            //check to see if we've scanned this section already

            for( Iterator<Directory> it = m_trees->iterator(); it != m_trees->end(); ++it )
            {
               if( new_path == (*it)->name8Bit() )
               {
                  debug() << "Tree pre-completed: " << (*it)->name() << "\n";
                  d = it.remove();
                  ScanManager::s_files += d->children();
                  //**** ideally don't have this redundant extra somehow
                  cwd->append( d, new_dirname );
               }
            }

            if( !d ) //then scan
               if ((d = scan( new_path, new_dirname ))) //then scan was successful
                  cwd->append( d );
         }

         ++ScanManager::s_files;
      }

      closedir( dir );

      return cwd;
   }

   bool
   LocalLister::readMounts()
   {
      #define INFO_PARTITIONS "/proc/partitions"
      #define INFO_MOUNTED_PARTITIONS "/etc/mtab" /* on Linux... */

      //**** SHAMBLES
      //  ** mtab should have priority as mount points don't have to follow fstab
      //  ** no removable media detection
      //  ** no updates if mounts change
      //  ** you want a KDE extension that handles this for you really

      struct fstab *fstab_ent;
#ifdef HAVE_MNTENT_H
      struct mntent *mnt_ent;
#endif
      QString str;


#ifdef HAVE_MNTENT_H
      FILE *fp;
      if( setfsent() == 0 || !( fp = setmntent( INFO_MOUNTED_PARTITIONS, "r" ) ) )
#else
      if( setfsent() == 0 )
#endif
         return false;

      #define FS_NAME   fstab_ent->fs_spec    // device-name
      #define FS_FILE   fstab_ent->fs_file    // mount-point
      #define FS_TYPE   fstab_ent->fs_vfstype // fs-type
      #define FS_MNTOPS fstab_ent->fs_mntops  // mount-options

      QStringList remoteFsTypes;
      remoteFsTypes << "smbfs" ;
#ifdef MNTTYPE_NFS
      remoteFsTypes << MNTTYPE_NFS;
#else
      remoteFsTypes << "nfs";
#endif
      // What about afs?

      while( (fstab_ent = getfsent()) != NULL )
      {
         str = QString( FS_FILE );
         if( str == "/" ) continue;
         str += '/';

         if( remoteFsTypes.contains( FS_TYPE ) )
            s_remoteMounts.append( str ); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

         else
            s_localMounts.append( str ); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

         kdDebug() << "FSTAB: " << FS_TYPE << "\n";
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

#ifdef HAVE_MNTENT_H
      while( ( mnt_ent = getmntent( fp ) ) != NULL )
      {
         bool b = false;

         str = QString( FS_FILE );
         if( str == "/" ) continue;
         str += "/";

         if( remoteFsTypes.contains( FS_TYPE ) )
            if( b = !s_remoteMounts.contains( str ) )
            s_remoteMounts.append( str ); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

         else if( b = !s_localMounts.contains( str ) )
            s_localMounts.append( str ); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

         if( b ) kdDebug() << "MTAB: " << FS_TYPE << "\n";
      }

      endmntent( fp ); /* close mtab.. */
#endif


      return true;
   }
}
