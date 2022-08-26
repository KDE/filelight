/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2017-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "fileTree.h"

#include <KSandbox>
#include <QDir>
#include <QUrl>

#include "fileCleaner.h"

Folder::~Folder()
{
    // Trees can be partially destroyed, make sure we remove the reference up to us, it's not a smart pointer
    // because that'd risk causing a loop.
    if (!m_parent) {
        for (auto &file : files) {
            file->m_parent = nullptr;
        }
    }
    FileCleaner::instance()->clean(files);
}

QString File::displayName() const
{
    QString decodedName = QString::fromUtf8(m_name);
    if (KSandbox::isInside()) {
        return decodedName.remove(QLatin1String("/run/host"));
    }
    return url().isLocalFile() ? QDir::toNativeSeparators(decodedName) : decodedName;
}

QString File::displayPath(const std::shared_ptr<Folder> &root) const
{
    // Use QUrl to sanitize the path for display and then run it through
    // QDir to make sure we use native path separators.
    const QUrl url = this->url(root);
    QString cleanPath = url.toDisplayString(QUrl::PreferLocalFile | QUrl::NormalizePathSegments);
    if (KSandbox::isInside()) {
        return cleanPath.remove(QLatin1String("/run/host"));
    }
    return url.isLocalFile() ? QDir::toNativeSeparators(cleanPath) : cleanPath;
}

QUrl File::url(const std::shared_ptr<Folder> &root) const
{
    QString path;

    // prevent returning empty string when there is something we could return
    const auto rootPtr = root.get() != this ? root.get() : nullptr;
    for (const File *d = this; d != rootPtr && d; d = d->parent()) {
        path.prepend(QString::fromUtf8(d->name8Bit()));
    }

    return QUrl::fromUserInput(path, QString(), QUrl::AssumeLocalFile);
}
