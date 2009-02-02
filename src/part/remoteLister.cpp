/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <sandsmark@samfundet.no>
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

#include <QApplication>
#include <QLinkedList>
#include <QTimer>
#include <QWidget>

namespace Filelight
{

//TODO: delete all this stuff!

// You need to use a single DirLister.
// One per directory breaks KIO (seemingly) and also uses un-godly amounts of memory!

struct Store {

    typedef QLinkedList<Store*> List;

    /// location of the directory
    const KUrl url;
    /// the directory on which we are operating
    Directory *directory;
    /// so we can reference the parent store
    Store *parent;
    /// directories in this directory that need to be scanned before we can propagate()
    List stores;

    Store() : directory(0), parent(0) {}
    Store(const KUrl &u, const QString &name, Store *s)
            : url(u), directory(new Directory(name.toLocal8Bit() + '/')), parent(s) {}


    Store* propagate()
    {
        /// returns the next store available for scanning

        kDebug() << "propagate: " << url << endl;

        if (parent) {
            parent->directory->append(directory);
            if (parent->stores.isEmpty()) {
                return parent->propagate();
            }
            else
                return parent;
        }

        //we reached the root, let's get our next directory scanned
        return this;
    }

private:
    Store(Store&);
    Store &operator=(const Store&);
};


RemoteLister::RemoteLister(const KUrl &url, QWidget *parent)
        : KDirLister(parent)
        , m_root(new Store(url, url.url(), 0))
        , m_store(m_root)
{
    setAutoUpdate(false); // Don't use KDirWatchers
    setShowingDotFiles(true); // Stupid KDirLister API function names
    setMainWindow(parent);

    // Use SIGNAL(result(KIO::Job*)) instead and then use Job::error()
    connect(this, SIGNAL(completed()), SLOT(completed()));
    connect(this, SIGNAL(canceled()), SLOT(canceled()));

    // We do this non-recursively - it is the only way!
    openUrl(url);
}

RemoteLister::~RemoteLister()
{
//    Directory *tree = isFinished() ? m_store->directory : 0;

//      qobject_cast<ScanManager*>(parent())->appendTree(tree, false);//FIXME TODO FUCK
    delete m_root;
}

void
RemoteLister::completed()
{
    kDebug() << "completed: " << url().prettyUrl() << endl;

    //as usual KDE documentation didn't suggest I needed to do this at all
    //I had to figure it out myself
    // -- avoid crash
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
    //m_directory is set to the directory we should operate on

    KFileItemList items = KDirLister::items();
    for (KFileItemList::ConstIterator it = items.begin(), end = items.end(); it != end; ++it)
    {
        if (it->isDir())
            m_store->stores += new Store(it->url(), it->name(), m_store);
        else
            m_store->directory->append(it->name().toLocal8Bit(), it->size() / 1024);

        ScanManager::s_files++;
    }


    if (m_store->stores.isEmpty())
        //no directories to scan, so we need to append ourselves to the parent directory
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
