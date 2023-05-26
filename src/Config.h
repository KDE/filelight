/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <QObject>
#include <QSet>
#include <QStringList>

namespace Filelight
{
Q_NAMESPACE
enum class Dirty {
    Layout = 1,
    Colors = 3,
};
Q_ENUM_NS(Dirty)

enum MapScheme { Rainbow, KDE, HighContrast };
Q_ENUM_NS(MapScheme)
} // namespace Filelight

Q_DECLARE_METATYPE(Filelight::Dirty)

class Config : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool scanAcrossMounts MEMBER scanAcrossMounts NOTIFY changed)
    Q_PROPERTY(bool scanRemoteMounts MEMBER scanRemoteMounts NOTIFY changed)
    Q_PROPERTY(bool showSmallFiles MEMBER showSmallFiles NOTIFY changed)
    Q_PROPERTY(uint contrast MEMBER contrast NOTIFY changed)
    Q_PROPERTY(uint defaultRingDepth MEMBER defaultRingDepth NOTIFY changed)
    Q_PROPERTY(Filelight::MapScheme scheme MEMBER scheme NOTIFY changed)
    Q_PROPERTY(QStringList skipList MEMBER skipList NOTIFY changed)
public:
    static Config *instance();
    void read();
    Q_INVOKABLE void write() const;
    Q_SIGNAL void changed();

    Q_INVOKABLE void addFolder();
    Q_INVOKABLE void removeFolder(const QString &url);

    // keep everything positive, avoid using DON'T, NOT or NO

    bool scanAcrossMounts;
    bool scanRemoteMounts;
    bool showSmallFiles;
    uint contrast;
    uint defaultRingDepth = 4;

    Filelight::MapScheme scheme;
    QStringList skipList;

    const QSet<QByteArray> remoteFsTypes = {"smbfs", "nfs", "afs"};
};
