/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef SCAN_H
#define SCAN_H

#include <QObject>
#include <QMutex>
#include <QList>

class Folder;

namespace Filelight
{

class LocalLister;

class ScanManager : public QObject
{
    Q_OBJECT

    friend class LocalLister;
    friend class RemoteLister;

public:
    explicit ScanManager(QObject *parent);
    ~ScanManager() override;

    bool start(const QUrl& path);
    bool running() const;

    int files() const {
        return m_files.loadRelaxed();
    }
    size_t totalSize() const {
        return m_totalSize.loadRelaxed();
    }

    void invalidateCacheFor(const QUrl &url);

public Q_SLOTS:
    bool abort();
    void emptyCache();
    void cacheTree(Folder*);
    void foundCached(Folder*);

Q_SIGNALS:
    void completed(Folder*);
    void aboutToEmptyCache();
    void branchCacheHit(Folder* tree);

private:
    bool m_abort;
    QAtomicInt m_files;
    QAtomicInteger<size_t> m_totalSize;

    QMutex m_mutex;
    LocalLister *m_thread;
    QList<Folder*> m_cache;
};
}

#endif
