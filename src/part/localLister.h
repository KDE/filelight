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

#ifndef LOCALLISTER_H
#define LOCALLISTER_H

#include <QThread>
#include <Q3CString>

class Directory;
template<class T> class Chain;

namespace Filelight
{
class LocalLister : public QThread
{
    Q_OBJECT

public:
    LocalLister(const QString &path, Chain<Directory> *cachedTrees, QObject *parent);

    static bool readMounts();

signals:
    void branchCompleted(Directory* tree, bool finished);

private:
    QString m_path;
    Chain<Directory> *m_trees;
    QObject *m_parent;

private:
    virtual void run();
    Directory *scan(const Q3CString&, const Q3CString&);

private:
    static QStringList s_localMounts, s_remoteMounts; //TODO namespace
};
}

#endif
