// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QQmlEngine>

class Folder;
class File;

namespace Filelight
{

class FileModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QUrl url READ url NOTIFY treeChanged /* derives from tree */)

public:
    static FileModel *instance();
    static FileModel *create(QQmlEngine *qml, QJSEngine *js);

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

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int intRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE [[nodiscard]] std::shared_ptr<File> file(int row) const;

private:
    // Fun fact: Qt's moc fails to deal with the `using` keyword properly.
    // If we were using QAbstractListModel::QAbstractListModel it'd not use the
    // create() function for some reason.
    explicit FileModel(QObject *parent = nullptr);
};

} // namespace Filelight
