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

#include "remoteLister.h"
#include "fileTree.h"
#include "scan.h"

#include <KDirLister>

#include <QLinkedList>
#include <QTimer>
#include <QWidget>

namespace Filelight
{

//TODO: delete all this stuff!

// You need to use a single DirLister.
// One per folder breaks KIO (seemingly) and also uses un-godly amounts of memory!

struct Store {

    typedef QLinkedList<Store*> List;

    /// location of the folder
    const QUrl url;
    /// the folder on which we are operating
    Folder *folder = nullptr;
    /// so we can reference the parent store
    Store *parent = nullptr;
    /// directories in this folder that need to be scanned before we can propagate()
    List stores;

    Store(const QUrl &u, const QString &name, Store *s)
            : url(u), folder(new Folder(name.toUtf8() + '/')), parent(s) { }


    Store* propagate()
    {
        /// returns the next store available for scanning

        qDebug() << "propagate: " << url;

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
        , m_root(new Store(url, url.url(), 0))
        , m_store(m_root)
        , m_manager(manager)
{
    setShowingDotFiles(true); // Stupid KDirLister API function names
    setMainWindow(parent);

    // Use SIGNAL(result(KIO::Job*)) instead and then use Job::error()
    connect(this, &RemoteLister::completed, this, &RemoteLister::completed);
    connect(this, &RemoteLister::canceled, this, &RemoteLister::canceled);
}

RemoteLister::~RemoteLister()
{
    Folder *tree = isFinished() ? m_store->folder : 0;

    emit branchCompleted(tree, false);
    delete m_root;
}

void
RemoteLister::completed()
{
    qDebug() << "completed: " << url().toString();

    // Delay the call to _completed since it can do a "delete this"
    QTimer::singleShot(0, this, SLOT(_completed()));
}

void
RemoteLister::canceled()
{
    qDebug() << "canceled: " << url().toString();

    QTimer::singleShot(0, this, SLOT(_completed()));
}

void
RemoteLister::_completed()
{
    //m_folder is set to the folder we should operate on

    const KFileItemList items = KDirLister::items();
    for (KFileItemList::ConstIterator it = items.begin(), end = items.end(); it != end; ++it)
    {
        if (it->isDir())
            m_store->stores += new Store(it->url(), it->name(), m_store);
        else
            m_store->folder->append(it->name().toUtf8(), it->size());

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
        qDebug() << "scanning: " << url;
        openUrl(url);
    }
    else {
        qDebug() << "I think we're done";

        Q_ASSERT(m_root == m_store);

        delete this;
    }
}
}


