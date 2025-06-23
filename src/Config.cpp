/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "Config.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QDebug>
#include <QFileDialog>
#include <QFont>

void Config::read()
{
    const KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("filelight_part"));

    scanAcrossMounts = config.readEntry("scanAcrossMounts", false);
    scanRemoteMounts = config.readEntry("scanRemoteMounts", false);
    showSmallFiles = config.readEntry("showSmallFiles", false);
    showFoldersSidebar = config.readEntry("showFoldersSidebar", true);
    contrast = config.readEntry("contrast", 75);
    scheme = (Filelight::MapScheme)config.readEntry("scheme", 0);
    skipList = config.readEntry("skipList", QStringList());

    Q_EMIT changed();
}

void Config::write() const
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("filelight_part"));

    config.writeEntry("scanAcrossMounts", scanAcrossMounts);
    config.writeEntry("scanRemoteMounts", scanRemoteMounts);
    config.writeEntry("showSmallFiles", showSmallFiles);
    config.writeEntry("showFoldersSidebar", showFoldersSidebar);
    config.writeEntry("contrast", contrast);
    config.writeEntry("scheme", (int)scheme); // TODO: make the enum belong to a qwidget,
    // and use magic macros to make it save this properly
    config.writePathEntry("skipList", skipList);

    config.sync();
}

Config *Config::instance()
{
    static Config self;
    return &self;
}

void Config::addFolder()
{
    const QString urlString = QFileDialog::getExistingDirectory(nullptr, i18n("Select path to ignore"), QDir::rootPath());
    const QUrl url = QUrl::fromLocalFile(urlString);

    // TODO error handling!
    // TODO wrong protocol handling!

    if (!url.isEmpty()) {
        const QString path = url.toLocalFile();

        if (!skipList.contains(path)) {
            skipList.append(path);
            Q_EMIT changed();
        } else {
            Q_EMIT addFolderFailed(i18n("Folder already ignored"));
        }
    }
}

void Config::removeFolder(const QString &url)
{
    qDebug() << url;
    skipList.removeAll(url);
    Q_EMIT changed();
}
