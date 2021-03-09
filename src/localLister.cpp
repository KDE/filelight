/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "localLister.h"

#include "Config.h"
#include "fileTree.h"
#include "scan.h"
#include "filelight_debug.h"

#include <QStorageInfo>
#include <QElapsedTimer>
#include <QGuiApplication> //postEvent()
#include <QFile>

#include <dirent.h>
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

    for (const QString &ignorePath : qAsConst(list)) {
        if (ignorePath.startsWith(path)) {
            QString folderName = ignorePath;
            if (!folderName.endsWith(QLatin1Char('/'))) {
                folderName += QLatin1Char('/');
            }
            m_trees->append(new Folder(folderName.toLocal8Bit().constData()));
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
    qCDebug(FILELIGHT_LOG) << "Scan completed in" << (timer.elapsed()/1000);

    //delete the list of trees useful for this scan,
    //in a successful scan the contents would now be transferred to 'tree'
    delete m_trees;

    if (m_parent->m_abort) //scan was cancelled
    {
        qCDebug(FILELIGHT_LOG) << "Scan successfully aborted";
        delete tree;
        tree = nullptr;
    }
    qCDebug(FILELIGHT_LOG) << "Emitting signal to cache results ...";
    Q_EMIT branchCompleted(tree);
    qCDebug(FILELIGHT_LOG) << "Thread terminating ...";
}

#ifndef S_BLKSIZE
#define S_BLKSIZE 512
#endif


#include <errno.h>
static void
outputError(const QByteArray &path)
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
    Folder *cwd = new Folder(dirname.constData());
    DIR *dir = opendir(path.constData());

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

        // QStringBuilder is used here. It assumes ent->d_name is char[NAME_MAX + 1],
        // and thus copies only first NAME_MAX + 1 chars.
        // Actually, while it's not fully POSIX-compatible, current behaviour may return d_name longer than NAME_MAX.
        // Make full copy of this string.
        QByteArray new_path = path + static_cast<const char*>(ent->d_name);

        //get file information
        if (lstat(new_path.constData(), &statbuf) == -1) {
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

        if (S_ISREG(statbuf.st_mode)) { //file
#ifndef Q_OS_WIN
            const size_t size = (statbuf.st_blocks * S_BLKSIZE);
#else
            const size_t size = statbuf.st_size;
#endif
            cwd->append(ent->d_name, size);
            m_parent->m_totalSize += size;
        }
        else if (S_ISDIR(statbuf.st_mode)) //folder
        {
            Folder *d = nullptr;
            const QByteArray new_dirname = QByteArray(ent->d_name) + QByteArrayLiteral("/");
            new_path += '/';

            //check to see if we've scanned this section already

            for (Folder *folder : *m_trees)
            {
                if (new_path == folder->name8Bit())
                {
                    qCDebug(FILELIGHT_LOG) << "Tree pre-completed: " << folder->decodedName();
                    d = folder;
                    m_trees->removeAll(folder);
                    m_parent->m_files += folder->children();
                    cwd->append(folder, new_dirname.constData());
                }
            }

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
    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (storage.isRoot()) {
            continue;
        }

        QString path = storage.rootPath();
        if (!path.endsWith(QLatin1Char('/'))) {
            path += QLatin1Char('/');
        }

        if (Config::remoteFsTypes.contains(storage.fileSystemType()) && !s_remoteMounts.contains(path)) {
            s_remoteMounts.append(path);
        } else if (!s_localMounts.contains(path)) {
            s_localMounts.append(path);
        }
    }

    qCDebug(FILELIGHT_LOG) << "Found the following remote filesystems: " << s_remoteMounts;
    qCDebug(FILELIGHT_LOG) << "Found the following local filesystems: " << s_localMounts;
}

}//namespace Filelight



