/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
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

#include <QStorageInfo>
#include <QElapsedTimer>
#include <QGuiApplication> //postEvent()
#include <QFile>
#include <QByteArray>

#include <dirent.h>
#ifdef Q_OS_SOLARIS
#include <sys/vfstab.h>
#elif !defined(Q_OS_WIN)
#include <fstab.h>
#endif
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

LocalLister::LocalLister(const QString &path, QList<Folder *> *cachedTrees, ScanManager *parent)
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

    foreach(const QString &ignorePath, list) {
        if (ignorePath.startsWith(path)) {
            m_trees->append(new Folder(ignorePath.toLocal8Bit()));
        }
    }
}

void
LocalLister::run()
{
    QElapsedTimer timer;
    timer.start();
    //recursively scan the requested path
    const QByteArray path = QFile::encodeName(m_path);
    Folder *tree = scan(path, path);
    qDebug() << "Scan completed in" << (timer.elapsed()/1000);

    //delete the list of trees useful for this scan,
    //in a sucessful scan the contents would now be transferred to 'tree'
    delete m_trees;

    if (m_parent->m_abort) //scan was cancelled
    {
        qDebug() << "Scan successfully aborted";
        delete tree;
        tree = 0;
    }
    qDebug() << "Emitting signal to cache results ...";
    emit branchCompleted(tree);
    qDebug() << "Thread terminating ...";
}

#ifndef S_BLKSIZE
#define S_BLKSIZE 512
#endif


#include <errno.h>
static void
outputError(QByteArray path)
{
    ///show error message that stat or opendir may give

#define out(s) qWarning() << s ": " << path; break

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
#ifndef Q_OS_WIN
    case ELOOP: //NOTE shouldn't ever happen
        out("Too many symbolic links encountered while traversing the path");
#endif
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

#ifdef _MSC_VER
//use a wider struct on win for nice handling of files larger than 2 GB
#undef KDE_struct_stat
#undef KDE_lstat
#define KDE_struct_stat struct _stat64
#define KDE_lstat kdewin32_stat64
#endif

    struct stat statbuf;
    dirent *ent;
    while ((ent = readdir(dir)))
    {
        if (m_parent->m_abort)
        {
            closedir(dir);
            return cwd;
        }

        if (qstrcmp(ent->d_name, ".") == 0 || qstrcmp(ent->d_name, "..") == 0)
            continue;

        QByteArray new_path = path + ent->d_name;

        //get file information
        if (lstat(new_path, &statbuf) == -1) {
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
#ifndef Q_OS_WIN
            cwd->append(ent->d_name, (statbuf.st_blocks * S_BLKSIZE));
#else
            cwd->append(ent->d_name, statbuf.st_size);
#endif

        else if (S_ISDIR(statbuf.st_mode)) //folder
        {
            Folder *d = 0;
            QByteArray new_dirname = ent->d_name;

            //check to see if we've scanned this section already

            for (Folder *folder : *m_trees)
            {
                if (new_path == folder->name8Bit())
                {
                    qDebug() << "Tree pre-completed: " << folder->name();
                    m_trees->removeAll(folder);
                    m_parent->m_files += folder->children();
                    cwd->append(folder, new_dirname);
                }
            }

            new_dirname += '/';
            new_path += '/';

            if (!d) //then scan
                if ((d = scan(new_path, new_dirname))) //then scan was successful
                    cwd->append(d);
        }

        ++m_parent->m_files;
    }

    closedir(dir);

    std::sort(cwd->files.begin(), cwd->files.end(), [](File *a, File*b) { return a->size() > b->size(); });

    return cwd;
}

void LocalLister::readMounts()
{
    static const QSet<QByteArray> remoteFsTypes = { "smbfs", "nfs", "afs" };

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (storage.isRoot()) {
            continue;
        }

        QString path = storage.rootPath();
        if (!path.endsWith(QLatin1Char('/'))) {
            path += QLatin1Char('/');
        }

        if (remoteFsTypes.contains(storage.fileSystemType()) && !s_remoteMounts.contains(path)) {
            s_remoteMounts.append(path);
        } else if (!s_localMounts.contains(path)) {
            s_localMounts.append(path);
        }
    }

    qDebug() << "Found the following remote filesystems: " << s_remoteMounts;
    qDebug() << "Found the following local filesystems: " << s_localMounts;
}

}//namespace Filelight



