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

#include "scan.h"

#include "remoteLister.h"
#include "fileTree.h"
#include "localLister.h"

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
        qDebug() << "Attempting to abort scan operation...";
        m_abort = true;
        m_thread->wait();
    }

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

    qDebug() << "Scan requested for: " << url;

    if (running()) {
        qWarning() << "Tried to launch two concurrent scans, aborting old one...";
        abort();
    }

    m_files = 0;
    m_abort = false;

    if (!url.isLocalFile()) {
        QGuiApplication::changeOverrideCursor(Qt::BusyCursor);
        //will start listing straight away
        Filelight::RemoteLister *remoteLister = new Filelight::RemoteLister(url, (QWidget*)parent(), this);
        connect(remoteLister, &Filelight::RemoteLister::branchCompleted, this, &ScanManager::cacheTree, Qt::QueuedConnection);
        remoteLister->setParent(this);
        remoteLister->setObjectName(QLatin1String( "remote_lister" ));
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

            qDebug() << "Cache-(a)hit: " << cachePath;

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
                qDebug() << "Found cache-handle, generating map..";

                emit branchCacheHit(d);

                return true;
            } else {
                //something went wrong, we couldn't find the folder we were expecting
                qWarning() << "Didn't find " << path << " in the cache!\n";
                it.remove();
                emit aboutToEmptyCache();
                delete folder;
                break; //do a full scan
            }
        }  else if (cachePath.startsWith(path)) { //then part of the requested tree is already scanned
            qDebug() << "Cache-(b)hit: " << cachePath;
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

    delete findChild<RemoteLister *>(QLatin1String( "remote_lister" ));

    return m_thread && m_thread->wait();
}

void ScanManager::emptyCache()
{
    m_abort = true;

    if (m_thread && m_thread->isRunning()) {
        m_thread->wait();
    }

    emit aboutToEmptyCache();

    qDeleteAll(m_cache);
    m_cache.clear();
}

void ScanManager::cacheTree(Folder *tree)
{
    QMutexLocker locker(&m_mutex); // This gets released once it is destroyed.

    if (m_thread) {
        qDebug() << "Waiting for thread to terminate ...";
        m_thread->wait();
        qDebug() << "Thread terminated!";
        delete m_thread; //note the lister deletes itself
        m_thread = nullptr;
    }

    emit completed(tree);

    if (tree) {
        //we don't cache foreign stuff
        //we don't recache stuff (thus only type 1000 events)
        m_cache.append(tree);
    } else { //scan failed
        qDeleteAll(m_cache);
        m_cache.clear();
    }

    QGuiApplication::restoreOverrideCursor();
}

void ScanManager::foundCached(Folder *tree)
{
    emit completed(tree);
    QGuiApplication::restoreOverrideCursor();
}


}


