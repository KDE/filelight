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
#include "filelight_debug.h"
#include "scan.h"

#include <QElapsedTimer>
#include <QGuiApplication> //postEvent()
#include <QSemaphore>
#include <QStorageInfo>
#include <QThreadPool>

#include "directoryIterator.h"

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
    // add empty directories for any mount points that are in the path
    // TODO empty directories is not ideal as adds to fileCount incorrectly

    QStringList list(Config::skipList);
    if (!Config::scanAcrossMounts)
        list += s_localMounts;
    if (!Config::scanRemoteMounts)
        list += s_remoteMounts;

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

void LocalLister::run()
{
    QElapsedTimer timer;
    timer.start();
    // recursively scan the requested path
    const QByteArray path = m_path.toUtf8();
    Folder *tree = scan(path, path);
    qCDebug(FILELIGHT_LOG) << "Scan completed in" << (timer.elapsed() / 1000);

    // delete the list of trees useful for this scan,
    // in a successful scan the contents would now be transferred to 'tree'
    delete m_trees;

    if (m_parent->m_abort) // scan was cancelled
    {
        qCDebug(FILELIGHT_LOG) << "Scan successfully aborted";
        delete tree;
        tree = nullptr;
    }
    qCDebug(FILELIGHT_LOG) << "Emitting signal to cache results ...";
    Q_EMIT branchCompleted(tree);
    qCDebug(FILELIGHT_LOG) << "Thread terminating ...";
}

Folder *LocalLister::scan(const QByteArray &path, const QByteArray &dirname)
{
    auto cwd = new Folder(dirname.constData());
    QVector<QPair<QByteArray, QByteArray>> subDirectories;

    for (const auto &entry : DirectoryIterator(path)) {
        if (m_parent->m_abort) {
            return cwd;
        }

        if (entry.isSkipable) {
            continue;
        }

        if (entry.isFile) {
            cwd->append(entry.name.constData(), entry.size);
            m_parent->m_totalSize += entry.size;
        } else if (entry.isDir) {
            Folder *d = nullptr;
            const QByteArray new_dirname = entry.name + QByteArrayLiteral("/");
            qDebug() << new_dirname;
            const QByteArray new_path = path + entry.name + '/';
            qDebug() << new_path;

            // check to see if we've scanned this section already

            QMutexLocker lock(&m_treeMutex);
            for (Folder *folder : std::as_const(*m_trees)) {
                if (new_path == folder->name8Bit()) {
                    qDebug() << "Tree pre-completed: " << folder->decodedName();
                    d = folder;
                    m_trees->removeAll(folder);
                    m_parent->m_files += folder->children();
                    cwd->append(folder, new_dirname.constData());
                }
            }
            lock.unlock();

            if (!d) { // then scan
                // qDebug() << "Tree fresh" <<new_path << new_dirname;
                subDirectories.append({new_path, new_dirname});
            }
        }

        ++m_parent->m_files;
    }

    // Scan all subdirectories, either in separate threads or immediately,
    // depending on how many free threads there are in the threadpool.
    // Yes, it isn't optimal, but it's better than nothing and pretty simple.
    QVector<Folder *> returnedCwds(subDirectories.count());
    QSemaphore semaphore;
    for (int i = 0; i < subDirectories.count(); i++) {
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
        if (d) { // then scan was successful
            cwd->append(d);
        }
    }

    std::sort(cwd->files.begin(), cwd->files.end(), [](File *a, File *b) {
        return a->size() > b->size();
    });

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

} // namespace Filelight
