// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "fileModel.h"

#include <QMetaEnum>
#include <QUrl>

#include "fileTree.h"

namespace Filelight
{

void FileModel::setTree(const std::shared_ptr<Folder> &tree)
{
    if (m_tree == tree) {
        return;
    }

    beginResetModel();
    m_tree = tree;
    Q_EMIT treeChanged();
    endResetModel();
}

QUrl FileModel::url() const
{
    if (m_tree) {
        return m_tree->url();
    }
    return {};
}

int FileModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (!m_tree) {
        return 0;
    }
    return m_tree->files.size();
}

QVariant FileModel::data(const QModelIndex &index, int intRole) const
{
    if (!index.isValid() || !m_tree) {
        return {};
    }
    const auto row = index.row();
    const auto &file = m_tree->files.at(row);
    switch (intRole) {
    case Qt::DisplayRole:
        return file->displayName();
    }

    switch (static_cast<Role>(intRole)) {
    case Role::HumanReadableSize:
        return file->humanReadableSize();
    case Role::IsFolder:
        return file->isFolder();
    case Role::URL:
        return file->url();
    case Role::Segment:
        return file->segment();
    }

    return {};
}

QHash<int, QByteArray> FileModel::roleNames() const
{
    static QHash<int, QByteArray> roles;
    if (!roles.isEmpty()) {
        return roles;
    }

    roles = QAbstractListModel::roleNames();
    const QMetaEnum roleEnum = QMetaEnum::fromType<Role>();
    for (int i = 0; i < roleEnum.keyCount(); ++i) {
        const int value = roleEnum.value(i);
        Q_ASSERT(value != -1);
        roles[static_cast<int>(value)] = QByteArray("ROLE_") + roleEnum.valueToKey(value);
    }
    return roles;
}

std::shared_ptr<File> FileModel::file(int row) const
{
    return m_tree->files.at(row);
}

} // namespace Filelight
