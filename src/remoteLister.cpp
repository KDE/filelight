/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "remoteLister.h"
#include "scan.h"
#include "filelight_debug.h"


#include <QList>
#include <QWidget>

namespace Filelight
{

//TODO: delete all this stuff!

// You need to use a single DirLister.
// One per folder breaks KIO (seemingly) and also uses un-godly amounts of memory!

struct Store {

    typedef QList<Store*> List;

    /// location of the folder
    const QUrl url;
    /// the folder on which we are operating
    Folder *folder = nullptr;
    /// so we can reference the parent store
    Store *parent = nullptr;
    /// directories in this folder that need to be scanned before we can propagate()
    List stores;

    Store(const QUrl &u, const QString &name, Store *s)
            : url(u), folder(new Folder((name + QLatin1Char('/')).toUtf8().constData())), parent(s) { }


    Store* propagate()
    {
        /// returns the next store available for scanning

        qCDebug(FILELIGHT_LOG) << "propagate: " << url;

        if (parent) {
            parent->folder->append(folder);
            if (parent->stores.isEmpty()) {
                return parent->propagate();
            }
            else
                return parent;
        }

        //we reached the root, let's get our next folder scanned
        return this;
    }

private:
    Store(Store&);
    Store &operator=(const Store&);
};


RemoteLister::RemoteLister(const QUrl &url, QWidget *parent, ScanManager* manager)
        : KDirLister(parent)
        , m_root(new Store(url, url.url(), nullptr))
        , m_store(m_root)
        , m_manager(manager)
{
    setShowingDotFiles(true); // Stupid KDirLister API function names
    setMainWindow(parent);

    // Use SIGNAL(result(KIO::Job*)) instead and then use Job::error()
    connect(this, static_cast<void (KCoreDirLister::*)()>(&KCoreDirLister::completed), this, &RemoteLister::onCompleted);
    connect(this, static_cast<void (KCoreDirLister::*)()>(&KCoreDirLister::canceled), this, &RemoteLister::onCanceled);
}

RemoteLister::~RemoteLister()
{
    delete m_root;
}

void RemoteLister::onCanceled()
{
    qCDebug(FILELIGHT_LOG) << "Canceled";
    Q_EMIT branchCompleted(nullptr);
    deleteLater();
}

void RemoteLister::onCompleted()
{
    //m_folder is set to the folder we should operate on

    const KFileItemList items = KDirLister::items();
    for (KFileItemList::ConstIterator it = items.begin(), end = items.end(); it != end; ++it)
    {
        if (it->isDir()) {
            m_store->stores += new Store(it->url(), it->name(), m_store);
        } else {
            m_store->folder->append(it->name().toUtf8().constData(), it->size());
            m_manager->m_totalSize += it->size();
        }

        m_manager->m_files++;
    }


    if (m_store->stores.isEmpty()) {
        //no directories to scan, so we need to append ourselves to the parent folder
        //propagate() will return the next ancestor that has stores left to be
        //scanned, or root if we are done
        Store *newStore = m_store->propagate();
        if (newStore != m_store) {
            // We need to clean up old stores
            delete m_store;
            m_store = newStore;
        }
    }

    if (!m_store->stores.isEmpty())
    {
        Store::List::Iterator first = m_store->stores.begin();
        const QUrl url((*first)->url);
        Store *currentStore = m_store;

        //we should operate with this store next time this function is called
        m_store = *first;

        //we don't want to handle this store again
        currentStore->stores.erase(first);

        //this returns _immediately_
        qCDebug(FILELIGHT_LOG) << "scanning: " << url;
        openUrl(url);
    }
    else {
        qCDebug(FILELIGHT_LOG) << "I think we're done";

        Q_ASSERT(m_root == m_store);
        Q_EMIT branchCompleted(m_store->folder);

        deleteLater();
    }
}
}


