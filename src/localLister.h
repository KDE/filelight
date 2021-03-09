/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef LOCALLISTER_H
#define LOCALLISTER_H

#include <QByteArray>
#include <QThread>

class Folder;

namespace Filelight
{
class ScanManager;

class LocalLister : public QThread
{
    Q_OBJECT

public:
    LocalLister(const QString &path, QList<Folder*> *cachedTrees, ScanManager *parent);

    static void readMounts();

Q_SIGNALS:
    void branchCompleted(Folder* tree);

private:
    QString m_path;
    QList<Folder*> *m_trees;
    ScanManager *m_parent;

private:
    void run() override;
    Folder *scan(const QByteArray&, const QByteArray&);

private:
    static QStringList s_localMounts, s_remoteMounts; //TODO namespace
};
}

#endif
