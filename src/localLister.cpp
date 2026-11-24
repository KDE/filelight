
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
#include <QGuiApplication>
#include <QSemaphore>
#include <QStorageInfo>
#include <QThreadPool>

#include "directoryEntryListing.h"

namespace Filelight
{
QStringList LocalLister::s_remoteMounts;
QStringList LocalLister::s_localMounts;

LocalLister::LocalLister(const QString &path, QList<std::shared_ptr<Folder>> *cachedTrees, ScanManager *parent)
    : m_path(path)
    , m_trees(cachedTrees)
    , m_parent(parent)
{
    // Add empty directories for mount points that should not be scanned.
    // TODO: Empty directories affect the file count.

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

            m_trees->append(
                std::make_shared<Folder>(folderName.toLocal8Bit().constData()));
        }
    }
}

void LocalLister::run()
{
    QElapsedTimer timer;
    timer.start();

    // Recursively scan the requested path.
    const QByteArray path = m_path.toUtf8();
    auto tree = scan(path, path);

    static constexpr auto msToS = 1000;
    qCDebug(FILELIGHT_LOG) << "Scan completed in" << (timer.elapsed() / msToS);

    // Release the cached tree list.
    delete m_trees;

    if (m_parent->m_abort) {
        qCDebug(FILELIGHT_LOG) << "Scan successfully aborted";
        tree = nullptr;
    }

    qCDebug(FILELIGHT_LOG) << "Emitting signal to cache results ...";
    Q_EMIT branchCompleted(tree);

    qCDebug(FILELIGHT_LOG) << "Thread terminating ...";
}

std::shared_ptr<Folder> LocalLister::scan(
    const QByteArray &path,
    const QByteArray &dirname)
{
    auto cwd = std::make_shared<Folder>(dirname.constData());

    QList<QPair<QByteArray, QByteArray>> subDirectories;

    // Keep scan statistics local to this recursive invocation.
    // This avoids concurrent updates to ScanManager's shared counters.
    decltype(m_parent->m_files) scannedFiles = 0;
    decltype(m_parent->m_totalSize) scannedSize = 0;

    bool aborted = false;

    for (const auto &entry : DirectoryEntryListing{.path = path}.generate()) {
        if (m_parent->m_abort) {
            aborted = true;
            break;
        }

        if (entry.isSkippable || entry.isDuplicate) {
            continue;
        }

        if (entry.isFile) {
            cwd->append(entry.name.constData(), entry.size);
            scannedSize += entry.size;
        } else if (entry.isDir) {
            std::shared_ptr<Folder> d = nullptr;

            const QByteArray new_dirname =
                entry.name + QByteArrayLiteral("/");

            const QByteArray new_path =
                path + entry.name + '/';

            // Check whether this directory has already been scanned.
            {
                QMutexLocker lock(&m_treeMutex);

                QList<std::shared_ptr<Folder>> toRemove;

                for (const auto &folder : std::as_const(*m_trees)) {
                    if (new_path == folder->name8Bit()) {
                        qCDebug(FILELIGHT_LOG)
                            << "Tree pre-completed:"
                            << folder->decodedName()
                            << folder.get();

                        d = folder;
                        toRemove << folder;

                        scannedFiles += folder->children();

                        cwd->append(folder, new_dirname.constData());
                    }
                }

                for (const auto &folder : std::as_const(toRemove)) {
                    m_trees->removeAll(folder);
                }
            }

            if (!d) {
                qCDebug(FILELIGHT_LOG)
                    << "Tree fresh"
                    << new_path
                    << new_dirname;

                subDirectories.append({new_path, new_dirname});
            }
        }

        ++scannedFiles;
    }

    // Scan subdirectories only if cancellation has not been requested.
    if (!aborted) {
        QList<std::shared_ptr<Folder>> returnedCwds(
            subDirectories.count());

        QSemaphore semaphore;

        for (int i = 0; i < subDirectories.count(); ++i) {
            auto scanSubdir = [this, i, &subDirectories,
                               &semaphore, &returnedCwds]() {
                returnedCwds[i] = scan(
                    subDirectories[i].first,
                    subDirectories[i].second);

                semaphore.release(1);
            };

            // Create the runnable explicitly to ensure correct
            // reference counting inside QThreadPool.
            auto runnable = QRunnable::create(scanSubdir);

            if (!QThreadPool::globalInstance()->tryStart(runnable)) {
                // Fix: tryStart() did not take ownership when it failed.
                // Delete the unused runnable before scanning synchronously.
                delete runnable;

                scanSubdir();
            }
        }

        semaphore.acquire(subDirectories.count());

        for (const auto &d : std::as_const(returnedCwds)) {
            if (d) {
                cwd->append(d);
            }
        }
    }

    // Merge local statistics into shared ScanManager counters.
    // All recursive invocations use the same mutex for these updates.
    {
        QMutexLocker lock(&m_treeMutex);

        m_parent->m_files += scannedFiles;
        m_parent->m_totalSize += scannedSize;
    }

    // Sort files by descending size.
    std::sort(
        cwd->files.begin(),
        cwd->files.end(),
        [](const auto &a, const auto &b) {
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

        if (Config::instance()->remoteFsTypes.contains(
                storage.fileSystemType())) {

            if (!s_remoteMounts.contains(path)) {
                s_remoteMounts.append(path);
            }
        } else if (!s_localMounts.contains(path)) {
            s_localMounts.append(path);
        }
    }

    qCDebug(FILELIGHT_LOG)
        << "Found the following remote filesystems:"
        << s_remoteMounts;

    qCDebug(FILELIGHT_LOG)
        << "Found the following local filesystems:"
        << s_localMounts;
}

} // namespace Filelight
