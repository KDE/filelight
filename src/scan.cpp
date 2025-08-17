/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "scan.h"

#include "fileTree.h"
#include "filelight_debug.h"

#include <QCursor>
#include <QDir>
#include <QGuiApplication>
#include <QStringBuilder>

#include "memory.h"

namespace Filelight
{

ScanManager::ScanManager(QObject *parent)
    : QObject(parent)
    , m_abort(false)
    , m_files(0)
{
    connect(this, &ScanManager::branchCacheHit, this, &ScanManager::foundCached, Qt::QueuedConnection);
    // Bit aggressive this. Completed is emitted per folder I think.
    connect(this, &ScanManager::completed, this, &ScanManager::runningChanged, Qt::QueuedConnection);
}

ScanManager::~ScanManager()
{
    if (m_thread) {
        qCDebug(FILELIGHT_LOG) << "Attempting to abort scan operation...";
        m_abort = true;
        m_thread->wait();
    }

    // RemoteListers are QObjects and get automatically deleted
}

bool ScanManager::running() const
{
    return (m_thread && m_thread->isRunning()) || (m_remoteLister && !m_remoteLister->isFinished());
}

bool ScanManager::start(const QUrl &url)
{
    QMutexLocker locker(&m_mutex); // The m_mutex gets released once locker is destroyed (goes out of scope).

    // Refresh mounts on each scan in case some have been mounted or unmounted, etc.
    Filelight::LocalLister::readMounts();

    // url is guaranteed clean and safe

    qCDebug(FILELIGHT_LOG) << "Scan requested for: " << url;

    if (running()) {
        qCWarning(FILELIGHT_LOG) << "Tried to launch two concurrent scans, aborting old one...";
        abort();
    }

    m_files = 0;
    m_totalSize = 0;
    m_abort = false;

    if (!url.isLocalFile()) {
        QGuiApplication::changeOverrideCursor(Qt::BusyCursor);
        // will start listing straight away
        m_remoteLister = make_shared_qobject<Filelight::RemoteLister>(url, this);
        connect(m_remoteLister.get(), &Filelight::RemoteLister::branchCompleted, this, &ScanManager::cacheTree, Qt::QueuedConnection);
        auto updateRunning = [this] {
            if (m_remoteLister && m_remoteLister->isFinished()) {
                m_remoteLister = nullptr;
                Q_EMIT runningChanged();
            }
        };
        connect(m_remoteLister.get(), &Filelight::RemoteLister::completed, this, updateRunning);
        connect(m_remoteLister.get(), &Filelight::RemoteLister::canceled, this, updateRunning);
        m_remoteLister->openUrl(url);
        Q_EMIT runningChanged();
        return true;
    }

    QString path = url.toLocalFile();

    // Cross-platform consideration: we get the path from a URL and in there the
    // separator is always the portable slash, as such toLocalFile will also get us a slash
    // separator, not the native one. i.e. on windows this is C:forwardslash not C:backslash
    // do not use QDir::separator!
    // https://bugs.kde.org/show_bug.cgi?id=450863
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }

    auto *trees = new QList<std::shared_ptr<Folder>>;

    /* CHECK CACHE
     *   user wants: /usr/local/
     *   cached:     /usr/
     *
     *   user wants: /usr/
     *   cached:     /usr/local/, /usr/include/
     */

    QMutableListIterator<std::shared_ptr<Folder>> it(m_cache);
    while (it.hasNext()) {
        auto folder = it.next();
        const QString cachePath = folder->decodedName();

        if (path.startsWith(cachePath)) { // then whole tree already scanned
            // find a pointer to the requested branch

            qCDebug(FILELIGHT_LOG) << "Cache-(a)hit: " << cachePath;
            QList<QStringView> split = QStringView(path).mid(cachePath.length()).split(QLatin1Char('/'));
            std::shared_ptr<Folder> d = folder;

            while (!split.isEmpty() && d != nullptr) { // if NULL we have got lost so abort!!
                if (split.first().isEmpty()) { // found the dir
                    break;
                }
                QString s = split.first() % QLatin1Char('/'); // % is the string concatenation operator for QStringBuilder

                QListIterator<std::shared_ptr<File>> it(d->files);
                d = nullptr;
                while (it.hasNext()) {
                    auto subfolder = it.next();
                    if (s == subfolder->decodedName()) {
                        d = std::dynamic_pointer_cast<Folder>(subfolder);
                        break;
                    }
                }

                split.pop_front();
            }

            if (d) {
                delete trees;

                // we found a completed tree, thus no need to scan
                qCDebug(FILELIGHT_LOG) << "Found cache-handle, generating map..";

                Q_EMIT branchCacheHit(d);

                return true;
            } // something went wrong, we couldn't find the folder we were expecting
            qCWarning(FILELIGHT_LOG) << "Didn't find " << path << " in the cache!\n";
            it.remove();
            Q_EMIT aboutToEmptyCache();
            break; // do a full scan
        }
        if (cachePath.startsWith(path)) { // then part of the requested tree is already scanned
            qCDebug(FILELIGHT_LOG) << "Cache-(b)hit: " << cachePath;
            it.remove();
            trees->append(folder);
        }
    }

    QGuiApplication::changeOverrideCursor(QCursor(Qt::BusyCursor));
    // starts listing by itself
    m_thread = make_shared_qobject<Filelight::LocalLister>(path, trees, this);
    connect(m_thread.get(), &LocalLister::branchCompleted, this, &ScanManager::cacheTree, Qt::QueuedConnection);
    m_thread->start();

    Q_EMIT runningChanged();

    return true;
}

bool ScanManager::abort()
{
    m_abort = true;

    if (m_remoteLister) {
        m_remoteLister->stop();
    }
    m_remoteLister = nullptr;
    const bool ret = m_thread && m_thread->wait();
    Q_EMIT runningChanged();

    Q_EMIT aborted();
    return ret;
}

void ScanManager::invalidateCacheFor(const QUrl &url)
{
    m_abort = true;

    if (m_thread && m_thread->isRunning()) {
        m_thread->wait();
    }

    if (!url.isLocalFile()) {
        qWarning() << "Remote cache clearing not implemented";
        return;
    }

    QString path = url.toLocalFile();
    // Do not use QDir::separator! The path is using / even on windows.
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }

    Q_EMIT aboutToEmptyCache();

    QMutableListIterator<std::shared_ptr<Folder>> cacheIterator(m_cache);
    QList<std::shared_ptr<Folder>> subCaches;
    QList<std::shared_ptr<Folder>> oldCache = m_cache;
    while (cacheIterator.hasNext()) {
        auto folder = cacheIterator.next();
        cacheIterator.remove();

        QString cachePath = folder->decodedName();

        if (!path.startsWith(cachePath)) {
            continue;
        }
        QList<QStringView> splitPath = QStringView(path).mid(cachePath.length()).split(QLatin1Char('/'));
        auto d = folder;

        while (!splitPath.isEmpty() && d != nullptr) { // if NULL we have got lost so abort!!
            if (splitPath.first().isEmpty()) { // found the dir
                break;
            }
            QString wantedName = splitPath.takeFirst() % QLatin1Char('/'); // % is the string concatenation operator for QStringBuilder

            QListIterator<std::shared_ptr<File>> it(d->files);
            d = nullptr;
            while (it.hasNext()) {
                auto subfolder = it.next();
                if (subfolder->decodedName() == wantedName) {
                    // This is the one we want to remove
                    continue;
                }
                if (!subfolder->isFolder()) {
                    continue;
                }

                auto newFolder = std::dynamic_pointer_cast<Folder>(subfolder)->duplicate();
                newFolder->setName((cachePath.toLocal8Bit() + subfolder->name8Bit()));
                subCaches.append(newFolder);
                d = nullptr;
            }
        }

        if (!d || !d->parent()) {
            continue;
        }
        d->parent()->remove(d);
    }

    for (const auto &folder : subCaches) {
        m_cache.append(folder);
    }
}

void ScanManager::emptyCache()
{
    m_abort = true;

    if (m_thread && m_thread->isRunning()) {
        m_thread->wait();
    }

    Q_EMIT aboutToEmptyCache();

    m_cache.clear();
}

void ScanManager::cacheTree(std::shared_ptr<Folder> tree)
{
    QMutexLocker locker(&m_mutex); // This gets released once it is destroyed.

    if (m_thread) {
        qCDebug(FILELIGHT_LOG) << "Waiting for thread to terminate ...";
        m_thread->wait();
        qCDebug(FILELIGHT_LOG) << "Thread terminated!";
        m_thread = nullptr;
    }

    Q_EMIT completed(tree);

    if (tree) {
        // we don't cache foreign stuff
        // we don't recache stuff (thus only type 1000 events)
        // we always just have one tree cached, so we really don't need a list..
        m_cache.append(tree);
    } else { // scan failed
        m_cache.clear();
    }

    QGuiApplication::restoreOverrideCursor();
}

void ScanManager::foundCached(std::shared_ptr<Folder> tree)
{
    Q_EMIT completed(tree);
    QGuiApplication::restoreOverrideCursor();
}

ScanManager *ScanManager::instance()
{
    static ScanManager manager(nullptr);
    return &manager;
}

ScanManager *ScanManager::create([[maybe_unused]] QQmlEngine *qml, [[maybe_unused]] QJSEngine *js)
{
    auto manager = instance();
    QQmlEngine::setObjectOwnership(manager, QQmlEngine::CppOwnership);
    return manager;
}

} // namespace Filelight
