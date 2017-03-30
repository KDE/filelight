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

#include "localLister.h"

#include "Config.h"
#include "fileTree.h"
#include "scan.h"

#include <QStorageInfo>
#include <QElapsedTimer>
#include <QGuiApplication> //postEvent()
#include <QFile>
#include <QByteArray>

namespace Filelight
{
QStringList LocalLister::s_remoteMounts;
QStringList LocalLister::s_localMounts;

LocalLister::LocalLister(const QString &path, QList<Folder *> *cachedTrees, ScanManager *parent)
        : QThread()
        , m_path(path)
        , m_trees(cachedTrees)
        , m_parent(parent)
{
    //add empty directories for any mount points that are in the path
    //TODO empty directories is not ideal as adds to fileCount incorrectly

    QStringList list(Config::skipList);
    if (!Config::scanAcrossMounts) list += s_localMounts;
    if (!Config::scanRemoteMounts) list += s_remoteMounts;

    foreach(const QString &ignorePath, list) {
        if (ignorePath.startsWith(path)) {
            QString folderName = ignorePath;
            if (!folderName.endsWith(QLatin1Char('/'))) {
                folderName += QLatin1Char('/');
            }
            qDebug() << "ignore path starts with path " << ignorePath << path;
            m_trees->append(new Folder(folderName.toLocal8Bit()));
        }
    }
}

void
LocalLister::run()
{
    QElapsedTimer timer;
    timer.start();
    //recursively scan the requested path
    const QByteArray path = QFile::encodeName(m_path);
    Folder *tree = scan(path, path);
    qDebug() << "Scan completed in" << (timer.elapsed()/1000);

    //delete the list of trees useful for this scan,
    //in a sucessful scan the contents would now be transferred to 'tree'
    delete m_trees;

    if (m_parent->isAborting()) //scan was cancelled
    {
        qDebug() << "Scan successfully aborted";
        delete tree;
        tree = 0;
    }
    qDebug() << "Emitting signal to cache results ...";
    emit branchCompleted(tree);
    qDebug() << "Thread terminating ...";
}

void LocalLister::readMounts()
{
    static const QSet<QByteArray> remoteFsTypes = { "smbfs", "nfs", "afs" };

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (storage.isRoot()) {
            continue;
        }

        QString path = storage.rootPath();
        if (!path.endsWith(QLatin1Char('/'))) {
            path += QLatin1Char('/');
        }

        if (remoteFsTypes.contains(storage.fileSystemType()) && !s_remoteMounts.contains(path)) {
            s_remoteMounts.append(path);
        } else if (!s_localMounts.contains(path)) {
            s_localMounts.append(path);
        }
    }

    qDebug() << "Found the following remote filesystems: " << s_remoteMounts;
    qDebug() << "Found the following local filesystems: " << s_localMounts;
}

QStringList LocalLister::localMounts()
{
    return s_localMounts;
}

QStringList LocalLister::remoteMounts()
{
    return s_remoteMounts;
}

}//namespace Filelight
