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

#ifndef SCAN_H
#define SCAN_H

#include <KUrl>
#include <QObject>
#include <QMutex>

class QThread;
class Directory;
template<class T> class Chain;

namespace Filelight
{
class ScanManager : public QObject
{
    Q_OBJECT

    friend class LocalLister;
    friend class RemoteLister;

public:
    ScanManager(QObject *parent);
    virtual ~ScanManager();

    bool start(const KUrl&);
    bool running() const;

    static uint files() {
        return s_files;
    }

public slots:
    bool abort();
    void emptyCache();
    void cacheTree(Directory*, bool);

signals:
    void completed(Directory*);
    void aboutToEmptyCache();
    void branchCompleted(Directory* tree, bool finished);

private:
    static bool s_abort;
    static uint s_files;

    KUrl m_url;
    QMutex m_mutex;
    QThread *m_thread;
    Chain<Directory> *m_cache;

};
}

#endif
