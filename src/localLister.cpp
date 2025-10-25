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

LocalLister::LocalLister(const QString &path, QList<std::shared_ptr<Folder>> *cachedTrees, ScanManager *parent)
    : m_path(path)
    , m_trees(cachedTrees)
    , m_parent(parent)
{
    // add empty directories for any mount points that are in the path
    // TODO empty directories is not ideal as adds to fileCount incorrectly

    QStringList list(Config::instance()->skipList);
    if (!Config::instance()->scanAcrossMounts) {
        list += s_localMounts;
    }
    if (!Config::instance()->scanRemoteMounts) {
        list += s_remoteMounts;
    }

    for (const QString &ignorePath : std::as_const(list)) {
        if (ignorePath.startsWith(path)) {
            QString folderName = ignorePath;
            if (!folderName.endsWith(QLatin1Char('/'))) {
                folderName += QLatin1Char('/');
            }
            m_trees->append(std::make_shared<Folder>(folderName.toLocal8Bit().constData()));
        }
    }
}

void LocalLister::run()
{
    QElapsedTimer timer;
    timer.start();
    // recursively scan the requested path
    const QByteArray path = m_path.toUtf8();
    auto tree = scan(path, path);

    static constexpr auto msToS = 1000; // not worth using std::chrono for this single line
    qCDebug(FILELIGHT_LOG) << "Scan completed in" << (timer.elapsed() / msToS);

    // delete the list of trees useful for this scan,
    // in a successful scan the contents would now be transferred to 'tree'
    delete m_trees;

    if (m_parent->m_abort) // scan was cancelled
    {
        qCDebug(FILELIGHT_LOG) << "Scan successfully aborted";
        tree = nullptr;
    }
    qCDebug(FILELIGHT_LOG) << "Emitting signal to cache results ...";
    Q_EMIT branchCompleted(tree);
    qCDebug(FILELIGHT_LOG) << "Thread terminating ...";
}

std::shared_ptr<Folder> LocalLister::scan(const QByteArray &path, const QByteArray &dirname)
{
    auto cwd = std::make_shared<Folder>(dirname.constData());
    QList<QPair<QByteArray, QByteArray>> subDirectories;

    for (const auto &entry : DirectoryIterator(path)) {
        if (m_parent->m_abort) {
            return cwd;
        }

        if (entry.isSkippable || entry.isDuplicate) {
            continue;
        }

        if (entry.isFile) {
            cwd->append(entry.name.constData(), entry.size);
            m_parent->m_totalSize += entry.size;
        } else if (entry.isDir) {
            std::shared_ptr<Folder> d = nullptr;
            const QByteArray new_dirname = entry.name + QByteArrayLiteral("/");
            const QByteArray new_path = path + entry.name + '/';

            // check to see if we've scanned this section already

            QMutexLocker lock(&m_treeMutex);
            QList<std::shared_ptr<Folder>> toRemove;
            for (const auto &folder : std::as_const(*m_trees)) {
                if (new_path == folder->name8Bit()) {
                    qCDebug(FILELIGHT_LOG) << "Tree pre-completed: " << folder->decodedName() << folder.get();
                    d = folder;
                    toRemove << folder;
                    m_parent->m_files += folder->children();
                    cwd->append(folder, new_dirname.constData());
                }
            }

            for (const auto &folder : std::as_const(toRemove)) {
                m_trees->removeAll(folder);
                // The **possibly** last shared_ptr is now in toRemove and will get cleaned up with it.
            }

            lock.unlock();

            if (!d) { // then scan
                qCDebug(FILELIGHT_LOG) << "Tree fresh" << new_path << new_dirname;
                subDirectories.append({new_path, new_dirname});
            }
        }

        ++m_parent->m_files;
    }

    // Scan all subdirectories, either in separate threads or immediately,
    // depending on how many free threads there are in the threadpool.
    // Yes, it isn't optimal, but it's better than nothing and pretty simple.
    QList<std::shared_ptr<Folder>> returnedCwds(subDirectories.count());
    QSemaphore semaphore;
    for (int i = 0; i < subDirectories.count(); i++) {
        // Capture as a lambda rather than a std::function to speed up calling
        auto scanSubdir = [this, i, &subDirectories, &semaphore, &returnedCwds]() {
            returnedCwds[i] = scan(subDirectories[i].first, subDirectories[i].second);
            semaphore.release(1);
        };
        // Workaround! Do not pass the function to tryStart and have it internally create a runnable.
        // The runnable will have incorrect ref counting resulting in failed assertions inside QThreadPool.
        // Instead create the runnable ourselves to hit the code paths with correct ref counting.
        // https://bugs.kde.org/show_bug.cgi?id=449688
        auto runnable = QRunnable::create(scanSubdir);
        if (!QThreadPool::globalInstance()->tryStart(runnable)) {
            scanSubdir();
        }
    }
    semaphore.acquire(subDirectories.count());
    for (const auto &d : std::as_const(returnedCwds)) {
        if (d) { // then scan was successful
            cwd->append(d);
        }
    }

    std::sort(cwd->files.begin(), cwd->files.end(), [](const auto &a, const auto &b) {
        return a->size() > b->size();
    });

    return cwd;
}

void LocalLister::readMounts()
{
    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : volumes) {
        if (storage.isRoot()) {
            continue;
        }

        QString path = storage.rootPath();
        if (!path.endsWith(QLatin1Char('/'))) {
            path += QLatin1Char('/');
        }

        if (Config::instance()->remoteFsTypes.contains(storage.fileSystemType()) && !s_remoteMounts.contains(path)) {
            s_remoteMounts.append(path);
        } else if (!s_localMounts.contains(path)) {
            s_localMounts.append(path);
        }
    }

    qCDebug(FILELIGHT_LOG) << "Found the following remote filesystems: " << s_remoteMounts;
    qCDebug(FILELIGHT_LOG) << "Found the following local filesystems: " << s_localMounts;
}

} // namespace Filelight
