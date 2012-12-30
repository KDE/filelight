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

#ifndef SCAN_H
#define SCAN_H

#include <KUrl>
#include <QtCore/QObject>
#include <QtCore/QMutex>

class QThread;
class Folder;
template<class T> class Chain;

namespace Filelight
{
class ScanManager : public QObject
{
    Q_OBJECT

    friend class LocalLister;
    friend class RemoteLister;

public:
    explicit ScanManager(QObject *parent);
    virtual ~ScanManager();

    bool start(const KUrl&);
    bool running() const;

    uint files() const {
        return m_files;
    }

public slots:
    bool abort();
    void emptyCache();
    void cacheTree(Folder*, bool);

signals:
    void completed(Folder*);
    void aboutToEmptyCache();
    void branchCompleted(Folder* tree, bool finished);

private:
    bool m_abort;
    uint m_files;

    KUrl m_url;
    QMutex m_mutex;
    QThread *m_thread;
    Chain<Folder> *m_cache;

};
}

#endif
