/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
* SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
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
#include <QThreadPool>
#include <QSemaphore>

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

    for (const QString &ignorePath : std::as_const(list)) {
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

    QVector<QPair<QByteArray, QByteArray>> subDirectories;

    struct stat statbuf;
    dirent *ent;
    while ((ent = readdir(dir)))
    {
        if (m_parent->m_abort)
        {
            closedir(dir);
            return cwd;
        }

        if (qstrcmp(ent->d_name, ".") == 0 || qstrcmp(ent->d_name, "..") == 0) {
            continue;
        }

        // QStringBuilder is used here. It assumes ent->d_name is char[NAME_MAX + 1],
        // and thus copies only first NAME_MAX + 1 chars.
        // Actually, while it's not fully POSIX-compatible, current behaviour may return d_name longer than NAME_MAX.
        // Make full copy of this string.
        QByteArray new_path = path + static_cast<const char*>(ent->d_name);

        // get file information. Split this per-OS. File attributes on windows/ntfs are special and not properly
        // represented through its _stat-like API.

#ifdef Q_OS_WINDOWS
        // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesexa
        FILE_BASIC_INFO basicInfo;
        WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
        BOOL result =
            GetFileAttributesExA(new_path.constData(), GetFileExInfoStandard,
                                 (LPVOID)&fileAttributeData);
        if (result == 0) {
            qWarning() << "failed to get attributes for" << new_path;
            continue;
        }

        const auto attributes = fileAttributeData.dwFileAttributes;
        // Reparse points are symlinks or NTFS junctions.
        const bool isSkipable = attributes & FILE_ATTRIBUTE_REPARSE_POINT || attributes & FILE_ATTRIBUTE_TEMPORARY;
        const bool isDir = attributes & FILE_ATTRIBUTE_DIRECTORY;
        const bool isFile = !isSkipable && !isDir; // fileness is implicit in win32 api
        ULARGE_INTEGER ulargeInt;
        ulargeInt.HighPart = fileAttributeData.nFileSizeHigh;
        ulargeInt.LowPart = fileAttributeData.nFileSizeLow;
        const auto size = ulargeInt.QuadPart;
#else
        if (lstat(new_path.constData(), &statbuf) == -1) {
            outputError(new_path);
            continue;
        }

        const bool isSkipable = S_ISLNK(statbuf.st_mode) ||
                S_ISCHR(statbuf.st_mode) ||
                S_ISBLK(statbuf.st_mode) ||
                S_ISFIFO(statbuf.st_mode)||
                S_ISSOCK(statbuf.st_mode);
        const bool isDir = S_ISDIR(statbuf.st_mode);
        const bool isFile =S_ISREG(statbuf.st_mode);
        const auto size = statbuf.st_blocks * S_BLKSIZE;
#endif

        if (isSkipable) {
            continue;
        }

        if (isFile) {
            cwd->append(ent->d_name, size);
            m_parent->m_totalSize += size;
        } else if (isDir) {
            Folder *d = nullptr;
            const QByteArray new_dirname = QByteArray(ent->d_name) + QByteArrayLiteral("/");
            new_path += '/';

            //check to see if we've scanned this section already

            QMutexLocker lock(&m_treeMutex);
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
            lock.unlock();

            if (!d) { //then scan
                subDirectories.append({new_path, new_dirname});
            }
        }

        ++m_parent->m_files;
    }

    closedir(dir);

    // Scan all subdirectories, either in separate threads or immediately,
    // depending on how many free threads there are in the threadpool.
    // Yes, it isn't optimal, but it's better than nothing and pretty simple.
    QVector<Folder*> returnedCwds(subDirectories.count());
    QSemaphore semaphore;
    for (int i=0; i<subDirectories.count(); i++) {
        std::function<void()> scanSubdir = [this, i, &subDirectories, &semaphore, &returnedCwds]() {
            returnedCwds[i] = scan(subDirectories[i].first, subDirectories[i].second);
            semaphore.release(1);
        };
        if (!QThreadPool::globalInstance()->tryStart(scanSubdir)) {
            scanSubdir();
        }
    }
    semaphore.acquire(subDirectories.count());
    for (Folder *d : std::as_const(returnedCwds)) {
        if (d) { //then scan was successful
            cwd->append(d);
        }
    }

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
