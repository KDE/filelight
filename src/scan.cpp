/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "scan.h"

#include "remoteLister.h"
#include "fileTree.h"
#include "localLister.h"
#include "filelight_debug.h"

#include <QGuiApplication>
#include <QCursor>
#include <QDir>
#include <QStringBuilder>

namespace Filelight
{

ScanManager::ScanManager(QObject *parent)
        : QObject(parent)
        , m_abort(false)
        , m_files(0)
        , m_mutex()
        , m_thread(nullptr)
{
    Filelight::LocalLister::readMounts();
    connect(this, &ScanManager::branchCacheHit, this, &ScanManager::foundCached, Qt::QueuedConnection);
}

ScanManager::~ScanManager()
{
    if (m_thread) {
        qCDebug(FILELIGHT_LOG) << "Attempting to abort scan operation...";
        m_abort = true;
        m_thread->wait();
    }
    qDeleteAll(m_cache);

    //RemoteListers are QObjects and get automatically deleted
}

bool ScanManager::running() const
{
    return m_thread && m_thread->isRunning();
}

bool ScanManager::start(const QUrl &url)
{
    QMutexLocker locker(&m_mutex); // The m_mutex gets released once locker is destroyed (goes out of scope).

    //url is guaranteed clean and safe

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
        //will start listing straight away
        Filelight::RemoteLister *remoteLister = new Filelight::RemoteLister(url, (QWidget*)parent(), this);
        connect(remoteLister, &Filelight::RemoteLister::branchCompleted, this, &ScanManager::cacheTree, Qt::QueuedConnection);
        remoteLister->setParent(this);
        remoteLister->setObjectName(QStringLiteral( "remote_lister" ));
        remoteLister->openUrl(url);
        return true;
    }

    QString path = url.toLocalFile();

    if (!path.endsWith(QDir::separator())) path += QDir::separator();

    QList<Folder*> *trees = new QList<Folder*>;

    /* CHECK CACHE
         *   user wants: /usr/local/
         *   cached:     /usr/
         *
         *   user wants: /usr/
         *   cached:     /usr/local/, /usr/include/
         */

    QMutableListIterator<Folder*> it(m_cache);
    while (it.hasNext()) {
        Folder *folder = it.next();
        QString cachePath = folder->decodedName();

        if (path.startsWith(cachePath)) { //then whole tree already scanned
            //find a pointer to the requested branch

            qCDebug(FILELIGHT_LOG) << "Cache-(a)hit: " << cachePath;

            QVector<QStringRef> split = path.midRef(cachePath.length()).split(QLatin1Char('/'));
            Folder *d = folder;

            while (!split.isEmpty() && d != nullptr) { //if NULL we have got lost so abort!!
                if (split.first().isEmpty()) { //found the dir
                    break;
                }
                QString s = split.first() % QLatin1Char('/'); // % is the string concatenation operator for QStringBuilder

                QListIterator<File*> it(d->files);
                d = nullptr;
                while (it.hasNext()) {
                    File *subfolder = it.next();
                    if (s == subfolder->decodedName()) {
                        d = (Folder*)subfolder;
                        break;
                    }
                }

                split.pop_front();
            }

            if (d) {
                delete trees;

                //we found a completed tree, thus no need to scan
                qCDebug(FILELIGHT_LOG) << "Found cache-handle, generating map..";

                Q_EMIT branchCacheHit(d);

                return true;
            } else {
                //something went wrong, we couldn't find the folder we were expecting
                qCWarning(FILELIGHT_LOG) << "Didn't find " << path << " in the cache!\n";
                it.remove();
                Q_EMIT aboutToEmptyCache();
                delete folder;
                break; //do a full scan
            }
        }  else if (cachePath.startsWith(path)) { //then part of the requested tree is already scanned
            qCDebug(FILELIGHT_LOG) << "Cache-(b)hit: " << cachePath;
            it.remove();
            trees->append(folder);
        }
    }

    QGuiApplication::changeOverrideCursor(QCursor(Qt::BusyCursor));
    //starts listing by itself
    m_thread = new Filelight::LocalLister(path, trees, this);
    connect(m_thread, &LocalLister::branchCompleted, this, &ScanManager::cacheTree, Qt::QueuedConnection);
    m_thread->start();

    return true;
}

bool ScanManager::abort()
{
    m_abort = true;

    delete findChild<RemoteLister *>(QStringLiteral( "remote_lister" ));

    return m_thread && m_thread->wait();
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
    if (!path.endsWith(QDir::separator())) path += QDir::separator();

    Q_EMIT aboutToEmptyCache();

    QMutableListIterator<Folder*> cacheIterator(m_cache);
    QList<Folder*> subCaches;
    QList<Folder*> oldCache = m_cache;
    while (cacheIterator.hasNext()) {
        Folder *folder = cacheIterator.next();
        cacheIterator.remove();

        QString cachePath = folder->decodedName();

        if (!path.startsWith(cachePath)) {
            continue;
        }

        QVector<QStringRef> splitPath = path.midRef(cachePath.length()).split(QLatin1Char('/'));
        Folder *d = folder;

        while (!splitPath.isEmpty() && d != nullptr) { //if NULL we have got lost so abort!!
            if (splitPath.first().isEmpty()) { //found the dir
                break;
            }
            QString wantedName = splitPath.takeFirst() % QLatin1Char('/'); // % is the string concatenation operator for QStringBuilder

            QListIterator<File*> it(d->files);
            d = nullptr;
            while (it.hasNext()) {
                File *subfolder = it.next();
                if (subfolder->decodedName() == wantedName) {
                    // This is the one we want to remove
                    continue;
                }
                if (!subfolder->isFolder()) {
                    continue;
                }

                Folder *newFolder = ((Folder*)subfolder)->duplicate();
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
    qDeleteAll(oldCache);

    for (Folder *folder : subCaches) {
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

    qDeleteAll(m_cache);
    m_cache.clear();
}

void ScanManager::cacheTree(Folder *tree)
{
    QMutexLocker locker(&m_mutex); // This gets released once it is destroyed.

    if (m_thread) {
        qCDebug(FILELIGHT_LOG) << "Waiting for thread to terminate ...";
        m_thread->wait();
        qCDebug(FILELIGHT_LOG) << "Thread terminated!";
        delete m_thread; //note the lister deletes itself
        m_thread = nullptr;
    }

    Q_EMIT completed(tree);

    if (tree) {
        //we don't cache foreign stuff
        //we don't recache stuff (thus only type 1000 events)
        //we always just have one tree cached, so we really don't need a list..
        m_cache.append(tree);
    } else { //scan failed
        qDeleteAll(m_cache);
        m_cache.clear();
    }

    QGuiApplication::restoreOverrideCursor();
}

void ScanManager::foundCached(Folder *tree)
{
    Q_EMIT completed(tree);
    QGuiApplication::restoreOverrideCursor();
}


}


