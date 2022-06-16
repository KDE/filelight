/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "remoteLister.h"
#include "filelight_debug.h"
#include "scan.h"

#include <QList>
#include <QWidget>

namespace Filelight
{

// You need to use a single DirLister.
// One per folder breaks KIO (seemingly) and also uses un-godly amounts of memory!

struct Store {
    /// location of the folder
    const QUrl url;
    /// the folder on which we are operating
    std::shared_ptr<Folder> folder;
    /// so we can reference the parent store
    const std::shared_ptr<Store> parent = nullptr;
    /// directories in this folder that need to be scanned before we can propagate()
    QList<std::shared_ptr<Store>> stores;

    Store(const QUrl &url_, const QString &name, const std::shared_ptr<Store> &parentStore)
        : url(url_)
        , folder(std::make_shared<Folder>((name + QLatin1Char('/')).toUtf8().constData()))
        , parent(parentStore)
    {
    }
    ~Store() = default;

private:
    Q_DISABLE_COPY_MOVE(Store)
};

std::shared_ptr<Store> propagate(Store *store, const std::shared_ptr<Store> &root)
{
    /// returns the next store available for scanning (or the root itself)

    while (store->parent) {
        qCDebug(FILELIGHT_LOG) << "propagate: " << store->url;
        store->parent->folder->append(store->folder);
        if (!store->parent->stores.isEmpty()) {
            return store->parent;
        }
        store = store->parent.get();
    }

    return root;
}

RemoteLister::RemoteLister(const QUrl &url, ScanManager *parent)
    : KDirLister(parent)
    , m_root(std::make_shared<Store>(url, url.url(), nullptr))
    , m_store(m_root)
    , m_manager(parent)
{
    setShowingDotFiles(true); // Stupid KDirLister API function names

    // Use SIGNAL(result(KIO::Job*)) instead and then use Job::error()
    connect(this, static_cast<void (KCoreDirLister::*)()>(&KCoreDirLister::completed), this, &RemoteLister::onCompleted);
    connect(this, static_cast<void (KCoreDirLister::*)()>(&KCoreDirLister::canceled), this, &RemoteLister::onCanceled);
}

void RemoteLister::onCanceled()
{
    qCDebug(FILELIGHT_LOG) << "Canceled";
    Q_EMIT branchCompleted(nullptr);
}

void RemoteLister::onCompleted()
{
    // m_folder is set to the folder we should operate on

    const KFileItemList items = KDirLister::items();
    for (const auto &item : items) {
        if (item.isDir()) {
            m_store->stores << std::make_shared<Store>(item.url(), item.name(), m_store);
        } else {
            m_store->folder->append(item.name().toUtf8().constData(), item.size());
            m_manager->m_totalSize += item.size();
        }

        m_manager->m_files++;
    }

    if (m_store->stores.isEmpty()) {
        // no directories to scan, so we need to append ourselves to the parent folder
        // propagate() will return the next ancestor that has stores left to be
        // scanned, or root if we are done
        if (auto newStore = propagate(m_store.get(), m_root); newStore != m_store) {
            // We need to clean up old stores
            m_store = newStore;
        }
    }

    if (!m_store->stores.isEmpty()) {
        // we should operate with this store next time this function is called
        m_store = m_store->stores.takeFirst();

        const auto url = m_store->url;
        qCDebug(FILELIGHT_LOG) << "scanning: " << url;
        openUrl(url);
        return;
    }

    qCDebug(FILELIGHT_LOG) << "I think we're done";
    Q_ASSERT(m_root == m_store);
    Q_EMIT branchCompleted(m_store->folder);
}

RemoteLister::~RemoteLister()
{
    stop();
}

} // namespace Filelight
