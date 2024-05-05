// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <memory>

#include <QAbstractListModel>

class Folder;
class FileWrapper;

namespace Filelight
{

class FileModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url NOTIFY treeChanged /* derives from tree */)

public:
    Q_SIGNAL void treeChanged();
    std::shared_ptr<Folder> m_tree;

    void setTree(const std::shared_ptr<Folder> &tree);

    [[nodiscard]] QUrl url() const;

public:
    enum class Role {
        HumanReadableSize = Qt::UserRole,
        IsFolder,
        URL,
        Segment,
    };
    Q_ENUM(Role)

    using QAbstractListModel::QAbstractListModel;

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int intRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE [[nodiscard]] FileWrapper file(int row) const;
};

} // namespace Filelight
