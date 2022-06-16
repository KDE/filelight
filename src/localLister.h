/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <QByteArray>
#include <QMutex>
#include <QThread>

class Folder;

namespace Filelight
{
class ScanManager;

class LocalLister : public QThread
{
    Q_OBJECT

public:
    LocalLister(const QString &path, QList<std::shared_ptr<Folder>> *cachedTrees, ScanManager *parent);

    static void readMounts();

Q_SIGNALS:
    void branchCompleted(std::shared_ptr<Folder> tree);

private:
    QString m_path;
    QMutex m_treeMutex;
    QList<std::shared_ptr<Folder>> *m_trees;
    ScanManager *m_parent;

private:
    void run() override;
    std::shared_ptr<Folder> scan(const QByteArray &, const QByteArray &);

private:
    static QStringList s_localMounts;
    static QStringList s_remoteMounts; // TODO namespace
};
} // namespace Filelight

namespace std
{
template<>
struct default_delete<Filelight::LocalLister> {
    void operator()(Filelight::LocalLister *ptr) const
    {
        // It's a QThread, delete on its own event loop, not the deleters. Ensures against data races.
        ptr->deleteLater();
    }
};
} // namespace std
