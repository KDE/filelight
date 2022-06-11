// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "dropperItem.h"

#include <KUrlMimeData>

namespace Filelight
{

DropperItem::DropperItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemAcceptsDrops, true);
}

void DropperItem::dropEvent(QDropEvent *e)
{
    if (const QList<QUrl> uriList = KUrlMimeData::urlsFromMimeData(e->mimeData()); !uriList.isEmpty()) {
        Q_EMIT urlsDropped(uriList);
    }
}

void DropperItem::dragEnterEvent(QDragEnterEvent *e)
{
    dragMoveEvent(e);
}

void DropperItem::dragMoveEvent(QDragMoveEvent *e)
{
    if (const QList<QUrl> uriList = KUrlMimeData::urlsFromMimeData(e->mimeData()); !uriList.isEmpty()) {
        e->acceptProposedAction();
    }
}

} // namespace Filelight
