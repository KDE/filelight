/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "Config.h"

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QFont>

bool Config::scanAcrossMounts;
bool Config::scanRemoteMounts;
bool Config::showSmallFiles;
uint Config::contrast;
uint Config::defaultRingDepth;
Filelight::MapScheme Config::scheme;
QStringList Config::skipList;
const QSet<QByteArray> Config::remoteFsTypes = {"smbfs", "nfs", "afs"};

void Filelight::Config::read()
{
    const KConfigGroup config = KSharedConfig::openConfig()->group("filelight_part");

    scanAcrossMounts = config.readEntry("scanAcrossMounts", false);
    scanRemoteMounts = config.readEntry("scanRemoteMounts", false);
    showSmallFiles = config.readEntry("showSmallFiles", false);
    contrast = config.readEntry("contrast", 75);
    scheme = (MapScheme)config.readEntry("scheme", 0);
    skipList = config.readEntry("skipList", QStringList());

    defaultRingDepth = 4;
}

void Filelight::Config::write()
{
    KConfigGroup config = KSharedConfig::openConfig()->group("filelight_part");

    config.writeEntry("scanAcrossMounts", scanAcrossMounts);
    config.writeEntry("scanRemoteMounts", scanRemoteMounts);
    config.writeEntry("showSmallFiles", showSmallFiles);
    config.writeEntry("contrast", contrast);
    config.writeEntry("scheme", (int)scheme); // TODO: make the enum belong to a qwidget,
    // and use magic macros to make it save this properly
    config.writePathEntry("skipList", skipList);
}
