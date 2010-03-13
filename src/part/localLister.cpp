/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <sandsmark@samfundet.no>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#include "localLister.h"

#include "Config.h"
#include "fileTree.h"
#include "scan.h"

#include <KDebug>

#include <QtGui/QApplication> //postEvent()
#include <QtCore/QFile>
#include <QtCore/QByteArray>

#include <kde_file.h>
#include <dirent.h>
#include <fstab.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif

namespace Filelight
{
QStringList LocalLister::s_remoteMounts;
QStringList LocalLister::s_localMounts;

LocalLister::LocalLister(const QString &path, Chain<Folder> *cachedTrees, ScanManager *parent)
        : QThread()
        , m_path(path)
        , m_trees(cachedTrees)
        , m_parent(parent)
{
    //add empty directories for any mount points that are in the path
    //TODO empty directories is not ideal as adds to fileCount incorrectly

    QStringList list(Config::skipList);
    if (!Config::scanAcrossMounts) list += s_localMounts;
    if (!Config::scanRemoteMounts) list += s_remoteMounts;

    for (QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
        if ((*it).startsWith(path))
            //prevent scanning of these directories
            m_trees->append(new Folder((*it).toUtf8()));
}

void
LocalLister::run()
{
    //recursively scan the requested path
    const QByteArray path = QFile::encodeName(m_path);
    Folder *tree = scan(path, path);

    //delete the list of trees useful for this scan,
    //in a sucessful scan the contents would now be transferred to 'tree'
    delete m_trees;

    if (m_parent->m_abort) //scan was cancelled
    {
        kDebug() << "Scan successfully aborted";
        delete tree;
        tree = 0;
    }
    kDebug() << "Emitting signal to cache results ...";
    emit branchCompleted(tree, true);
    kDebug() << "Thread terminating ...";
}

// from system.h in GNU coreutils package
/* Extract or fake data from a `struct stat'.
ST_NBLOCKS: Number of blocks in the file, including indirect blocks.
ST_NBLOCKSIZE: Size of blocks used when calculating ST_NBLOCKS.  */
#define ST_NBLOCKS(statbuf) ((statbuf).st_size / ST_NBLOCKSIZE + ((statbuf).st_size % ST_NBLOCKSIZE != 0))

#ifndef ST_NBLOCKSIZE
#define ST_NBLOCKSIZE 512
#endif


#include <errno.h>
static void
outputError(QByteArray path)
{
    ///show error message that stat or opendir may give

#define out(s) kError() << s ": " << path; break

    switch (errno) {
    case EACCES:
        out("Inadequate access permissions");
    case EMFILE:
        out("Too many file descriptors in use by Filelight");
    case ENFILE:
        out("Too many files are currently open in the system");
    case ENOENT:
        out("A component of the path does not exist, or the path is an empty string");
    case ENOMEM:
        out("Insufficient memory to complete the operation");
    case ENOTDIR:
        out("A component of the path is not a folder");
    case EBADF:
        out("Bad file descriptor");
    case EFAULT:
        out("Bad address");
    case ELOOP: //NOTE shouldn't ever happen
        out("Too many symbolic links encountered while traversing the path");
    case ENAMETOOLONG:
        out("File name too long");
    }

#undef out
}

Folder*
LocalLister::scan(const QByteArray &path, const QByteArray &dirname)
{
    Folder *cwd = new Folder(dirname);
    DIR *dir = opendir(path);

    if (!dir) {
        outputError(path);
        return cwd;
    }

    KDE_struct_stat statbuf;
    dirent *ent;
    while ((ent = KDE_readdir(dir)))
    {
        if (m_parent->m_abort)
            return cwd;

        if (qstrcmp(ent->d_name, ".") == 0 || qstrcmp(ent->d_name, "..") == 0)
            continue;

        QByteArray new_path = path;
        new_path += ent->d_name;

        //get file information
        if (KDE_lstat(new_path, &statbuf) == -1) {
            outputError(new_path);
            continue;
        }

        if (S_ISLNK(statbuf.st_mode) ||
                S_ISCHR(statbuf.st_mode) ||
                S_ISBLK(statbuf.st_mode) ||
                S_ISFIFO(statbuf.st_mode)||
                S_ISSOCK(statbuf.st_mode))
        {
            continue;
        }

        if (S_ISREG(statbuf.st_mode)) //file
            //using units of KiB as 32bit max is 4GiB and 64bit ints are expensive
            cwd->append(ent->d_name, (ST_NBLOCKS(statbuf) * ST_NBLOCKSIZE) / 1024);

        else if (S_ISDIR(statbuf.st_mode)) //folder
        {
            Folder *d = 0;
            QByteArray new_dirname = ent->d_name;
            new_dirname += '/';
            new_path    += '/';

            //check to see if we've scanned this section already

            for (Iterator<Folder> it = m_trees->iterator(); it != m_trees->end(); ++it)
            {
                if (new_path == (*it)->name8Bit())
                {
                    kDebug() << "Tree pre-completed: " << (*it)->name();
                    d = it.remove();
                    m_parent->m_files += d->children();
                    //**** ideally don't have this redundant extra somehow
                    cwd->append(d, new_dirname);
                }
            }

            if (!d) //then scan
                if ((d = scan(new_path, new_dirname))) //then scan was successful
                    cwd->append(d);
        }

        ++m_parent->m_files;
    }

    closedir(dir);

    return cwd;
}

bool
LocalLister::readMounts()
{
#define INFO_PARTITIONS "/proc/partitions"
#define INFO_MOUNTED_PARTITIONS "/etc/mtab" /* on Linux... */

    ////////////
    //BIG FAT TODO TODO TODO
    // Use Solid for this

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
    if (setfsent() == 0 || !(fp = setmntent(INFO_MOUNTED_PARTITIONS, "r")))
#else
    if (setfsent() == 0)
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

    while ((fstab_ent = getfsent()) != NULL)
    {
        str = QString(FS_FILE);
        if (str == "/") continue;
        str += '/';

        if (remoteFsTypes.contains(FS_TYPE))
            s_remoteMounts.append(str); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

        else
            s_localMounts.append(str); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

        kDebug() << "FSTAB: " << FS_TYPE << "\n";
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
    while ((mnt_ent = getmntent(fp)) != NULL)
    {
        bool b = false;

        str = QString(FS_FILE);
        if (str == "/") continue;
        str += '/';

        if (remoteFsTypes.contains(FS_TYPE))
            if (b = !s_remoteMounts.contains(str))
                s_remoteMounts.append(str); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

            else if (b = !s_localMounts.contains(str))
                s_localMounts.append(str); //**** NO! can't be sure won't have trailing slash, need to do a check first dummy!!

        if (b) kDebug() << "MTAB: " << FS_TYPE << "\n";
    }

    endmntent(fp); /* Close mtab. */
#endif


    return true;
}
}

#include "localLister.moc"
