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

#include <KDebug>
#include <KDirLister>

#include <QtCore/QLinkedList>
#include <QtCore/QTimer>
#include <QtGui/QWidget>

namespace Filelight
{

//TODO: delete all this stuff!

// You need to use a single DirLister.
// One per folder breaks KIO (seemingly) and also uses un-godly amounts of memory!

struct Store {

    typedef QLinkedList<Store*> List;

    /// location of the folder
    const KUrl url;
    /// the folder on which we are operating
    Folder *folder;
    /// so we can reference the parent store
    Store *parent;
    /// directories in this folder that need to be scanned before we can propagate()
    List stores;

    Store() : folder(0), parent(0) {}
    Store(const KUrl &u, const QString &name, Store *s)
            : url(u), folder(new Folder(name.toUtf8() + '/')), parent(s) {}


    Store* propagate()
    {
        /// returns the next store available for scanning

        kDebug() << "propagate: " << url << endl;

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


RemoteLister::RemoteLister(const KUrl &url, QWidget *parent, ScanManager* manager)
        : KDirLister(parent)
        , m_root(new Store(url, url.url(), 0))
        , m_store(m_root)
        , m_manager(manager)
{
    setShowingDotFiles(true); // Stupid KDirLister API function names
    setMainWindow(parent);

    // Use SIGNAL(result(KIO::Job*)) instead and then use Job::error()
    connect(this, SIGNAL(completed()), SLOT(completed()));
    connect(this, SIGNAL(canceled()), SLOT(canceled()));
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
    kDebug() << "completed: " << url().prettyUrl() << endl;

    // Delay the call to _completed since it can do a "delete this"
    QTimer::singleShot(0, this, SLOT(_completed()));
}

void
RemoteLister::canceled()
{
    kDebug() << "canceled: " << url().prettyUrl() << endl;

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


    if (m_store->stores.isEmpty())
        //no directories to scan, so we need to append ourselves to the parent folder
        //propagate() will return the next ancestor that has stores left to be scanned, or root if we are done
        m_store = m_store->propagate();

    if (!m_store->stores.isEmpty())
    {
        Store::List::Iterator first = m_store->stores.begin();
        const KUrl url((*first)->url);
        Store *currentStore = m_store;

        //we should operate with this store next time this function is called
        m_store = *first;

        //we don't want to handle this store again
        currentStore->stores.erase(first);

        //this returns _immediately_
        kDebug() << "scanning: " << url << endl;
        openUrl(url);
    }
    else {
        kDebug() << "I think we're done\n";

        Q_ASSERT(m_root == m_store);

        delete this;
    }
}
}

#include "remoteLister.moc"
