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

#include <KDebug>

#include <QtGui/QApplication>

namespace Filelight
{

ScanManager::ScanManager(QObject *parent)
        : QObject(parent)
        , m_abort(false)
        , m_files(0)
        , m_mutex()
        , m_thread(0)
        , m_cache(new Chain<Folder>)
{
    Filelight::LocalLister::readMounts();
    connect(this, SIGNAL(branchCompleted(Folder*,bool)), this, SLOT(cacheTree(Folder*,bool)), Qt::QueuedConnection);
    connect(this, SIGNAL(branchCacheHit(Folder*)), this, SLOT(foundCached(Folder*)), Qt::QueuedConnection);
}

ScanManager::~ScanManager()
{
    if (m_thread) {
        kDebug() << "Attempting to abort scan operation..." << endl;
        m_abort = true;
        m_thread->wait();
    }

    delete m_cache;

    //RemoteListers are QObjects and get automatically deleted
}

bool ScanManager::running() const
{
    return m_thread && m_thread->isRunning();
}

bool ScanManager::start(const KUrl &url)
{
    QMutexLocker locker(&m_mutex); // The m_mutex gets released once locker is destroyed (goes out of scope).

    //url is guaranteed clean and safe

    kDebug() << "Scan requested for: " << url.prettyUrl() << endl;

    if (running()) {
        kWarning() << "Tried to launch two concurrent scans, aborting old one...";
        abort();
    }

    m_files = 0;
    m_abort = false;

    if (url.protocol() == QLatin1String( "file" ))
    {
        const QString path = url.path(KUrl::AddTrailingSlash);

        Chain<Folder> *trees = new Chain<Folder>;

        /* CHECK CACHE
        *   user wants: /usr/local/
        *   cached:     /usr/
        *
        *   user wants: /usr/
        *   cached:     /usr/local/, /usr/include/
        */

        for (Iterator<Folder> it = m_cache->iterator(); it != m_cache->end(); ++it)
        {
            QString cachePath = (*it)->name();

            if (path.startsWith(cachePath)) //then whole tree already scanned
            {
                //find a pointer to the requested branch

                kDebug() << "Cache-(a)hit: " << cachePath << endl;

                QStringList split = path.mid(cachePath.length()).split(QLatin1Char( '/' ));
                Folder *d = *it;
                Iterator<File> jt;

                while (!split.isEmpty() && d != NULL) //if NULL we have got lost so abort!!
                {
                    jt = d->iterator();

                    const Link<File> *end = d->end();
                    QString s = split.first();
                    if (s.isEmpty()) //found the dir
                        break;
                    s += QLatin1Char( '/' );

                    for (d = 0; jt != end; ++jt)
                        if (s == (*jt)->name())
                        {
                            d = (Folder*)*jt;
                            break;
                        }

                    split.pop_front();
                }

                if (d)
                {
                    delete trees;

                    //we found a completed tree, thus no need to scan
                    kDebug() << "Found cache-handle, generating map.." << endl;

                    emit branchCacheHit(d);

                    return true;
                }
                else
                {
                    //something went wrong, we couldn't find the folder we were expecting
                    kError() << "Didn't find " << path << " in the cache!\n";
                    delete it.remove(); //safest to get rid of it
                    break; //do a full scan
                }
            }
            else if (cachePath.startsWith(path)) //then part of the requested tree is already scanned
            {
                kDebug() << "Cache-(b)hit: " << cachePath << endl;
                it.transferTo(*trees);
            }
        }

        m_url.setPath(path); //FIXME stop switching between paths and KURLs all the time
        QApplication::setOverrideCursor(Qt::BusyCursor);
        //starts listing by itself
        m_thread = new Filelight::LocalLister(path, trees, this);
        connect(m_thread, SIGNAL(branchCompleted(Folder*,bool)), this, SLOT(cacheTree(Folder*,bool)), Qt::QueuedConnection);
        m_thread->start();

        return true;
    }

    m_url = url;
    QApplication::setOverrideCursor(Qt::BusyCursor);
    //will start listing straight away
    Filelight::RemoteLister *remoteLister = new Filelight::RemoteLister(url, (QWidget*)parent(), this);
    connect(remoteLister, SIGNAL(branchCompleted(Folder*,bool)), this, SLOT(cacheTree(Folder*,bool)), Qt::QueuedConnection);
    remoteLister->setParent(this);
    remoteLister->setObjectName(QLatin1String( "remote_lister" ));
    remoteLister->openUrl(url);
    return true;
}

bool
ScanManager::abort()
{
    m_abort = true;

    delete findChild<RemoteLister *>(QLatin1String( "remote_lister" ));

    return m_thread && m_thread->wait();
}

void
ScanManager::emptyCache()
{
    m_abort = true;

    if (m_thread && m_thread->isRunning())
        m_thread->wait();

    emit aboutToEmptyCache();

    m_cache->empty();
}

void
ScanManager::cacheTree(Folder *tree, bool finished)
{
    QMutexLocker locker(&m_mutex); // This gets released once it is destroyed.

    if (m_thread) {
        kDebug() << "Waiting for thread to terminate ...";
        m_thread->wait();
        kDebug() << "Thread terminated!";
        delete m_thread; //note the lister deletes itself
        m_thread = 0;
    }

    emit completed(tree);

    if (tree) {
        //we don't cache foreign stuff
        //we don't recache stuff (thus only type 1000 events)
        if (m_url.protocol() == QLatin1String( "file" ) && finished)
            //TODO sanity check the cache
            m_cache->append(tree);
    }
    else //scan failed
        m_cache->empty(); //FIXME this is safe but annoying

    QApplication::restoreOverrideCursor();
}

void
ScanManager::foundCached(Folder *tree)
{
    emit completed(tree);
    QApplication::restoreOverrideCursor();
}


}

#include "scan.moc"
