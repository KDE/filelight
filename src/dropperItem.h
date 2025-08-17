// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QQuickItem>

namespace Filelight
{

class DropperItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit DropperItem(QQuickItem *parent = nullptr);

    void dropEvent(QDropEvent *e) override final;
    void dragEnterEvent(QDragEnterEvent *e) override final;
    void dragMoveEvent(QDragMoveEvent *e) override final;

Q_SIGNALS:
    void urlsDropped(const QList<QUrl> &urls);
};

} // namespace Filelight
