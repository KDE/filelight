/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2017-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "fileTree.h"

#include <QDir>
#include <QUrl>

#include "fileCleaner.h"
#include "filelight_debug.h"

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
    const QString decodedName = QString::fromUtf8(m_name);
    return url().isLocalFile() ? QDir::toNativeSeparators(decodedName) : decodedName;
}

QString File::displayPath(const std::shared_ptr<Folder> &root) const
{
    // Use QUrl to sanitize the path for display and then run it through
    // QDir to make sure we use native path separators.
    const QUrl url = this->url(root);
    const QString cleanPath = url.toDisplayString(QUrl::PreferLocalFile | QUrl::NormalizePathSegments);
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

void Folder::clone(const Folder *that, std::shared_ptr<Folder> other)
{
    struct Clone {
        const Folder *source;
        std::shared_ptr<Folder> target;
        std::shared_ptr<Folder> parent;
    };
    QList<Clone> completedClones;
    QList<Clone> clones({{that, other, nullptr}});
    while (!clones.isEmpty()) {
        const auto &clone = clones.takeLast();
        qCDebug(FILELIGHT_LOG) << "cloning" << clone.source->m_name << clone.source << "into" << clone.target.get() << "parent" << clone.parent.get();
        for (const auto &file : clone.source->files) {
            qCDebug(FILELIGHT_LOG) << "  " << file->displayName() << file->isFolder();
            if (file->isFolder()) {
                auto folder = std::dynamic_pointer_cast<Folder>(file);
                clones.append(Clone{folder.get(), std::make_shared<Folder>(folder->m_name.constData()), clone.target});
            } else {
                clone.target->append(file->m_name.constData(), file->m_size);
            }
        }
        completedClones.append(clone);
    }
    for (auto it = completedClones.rbegin(); it != completedClones.rend(); ++it) {
        // Appending forwards the size data, it must only happen after all duplicating is done so the sizes are known.
        // And obviously in reverse order of existence in the tree.
        auto clone = *it;
        if (clone.parent) {
            clone.parent->append(clone.target);
        }
    }
}

std::shared_ptr<Folder> Folder::duplicate() const
{
    // We completely detach the subtree upon duplication, otherwise we'd have parent pointers pointing back into
    // the old tree causing major headaches when trying to keep clean object states. This does take longer
    // because we need to create additional objects, but is safer overall.
    auto other = std::make_shared<Folder>(m_name.constData());
    clone(this, other);
    return other;
}

bool File::isFilesGroup() const
{
    return false;
}
