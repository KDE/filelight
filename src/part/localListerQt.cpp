/***********************************************************************
* Copyright 2017  Harald Sitter <sitter@kde.org>
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

#include "localListerQt.h"

#include "Config.h"
#include "fileTree.h"
#include "scan.h"
#include "localLister.h"

#include <QElapsedTimer>
#include <QDir>
#include <QDirIterator>

namespace Filelight
{

Folder *LocalListerQt::scan(const QByteArray &path, const QByteArray &dirname)
{
    Folder *cwd = new Folder(dirname);

    // We do not want: symlinks, system files (fifo, socks, blks, chrs) nor lnk files on Windows.
    static QDir::Filters filter = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden;
    QDirIterator it(QString::fromLocal8Bit(path), filter);

    while (it.hasNext()) {
        it.next();
        const auto &info = it.fileInfo();
        if (m_parent->isAborting()) {
            return cwd;
        }

        QByteArray newPath = path + info.fileName().toLocal8Bit();

        if (info.isFile()) {
            cwd->append(info.fileName(), info.size());
        } else if (info.isDir()) {
            Folder *d = nullptr;
            QByteArray newDirName = info.fileName().toLocal8Bit();
            newPath += '/';

            // Check to see if we've scanned this section already

            for (Folder *folder : *m_trees) {
                if (newPath == folder->name8Bit()) {
                    qDebug() << "Tree pre-completed: " << folder->name();
                    d = folder;
                    m_trees->removeAll(folder);
                    m_parent->m_files += folder->children();
                    cwd->append(folder, newDirName);
                }
            }

            if (!d) //then scan
                if ((d = scan(newPath, newDirName + '/'))) //then scan was successful
                    cwd->append(d);
        }

        ++m_parent->m_files;
    }

    std::sort(cwd->files.begin(), cwd->files.end(), [](File *a, File*b) { return a->size() > b->size(); });

    return cwd;
}

}//namespace Filelight



