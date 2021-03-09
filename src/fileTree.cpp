/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
* SPDX-FileCopyrightText: 2017 Harald Sitter <sitter@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "fileTree.h"

#include <QDir>
#include <QUrl>

QString File::displayName() const {
    const QString decodedName = QFile::decodeName(m_name);
    return url().isLocalFile() ? QDir::toNativeSeparators(decodedName) : decodedName;
}

QString File::displayPath(const Folder *root) const
{
    // Use QUrl to sanitize the path for display and then run it through
    // QDir to make sure we use native path separators.
    const QUrl url = this->url(root);
    const QString cleanPath = url.toDisplayString(QUrl::PreferLocalFile | QUrl::NormalizePathSegments);
    return url.isLocalFile() ? QDir::toNativeSeparators(cleanPath) : cleanPath;
}

QUrl File::url(const Folder *root) const
{
    QString path;

    if (root == this) {
        root = nullptr; //prevent returning empty string when there is something we could return
    }

    for (const File *d = this; d != root && d; d = d->parent()) {
        path.prepend(QFile::decodeName(d->name8Bit()));
    }

    return QUrl::fromUserInput(path, QString(), QUrl::AssumeLocalFile);
}
