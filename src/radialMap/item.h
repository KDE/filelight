/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <QLabel>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QUrl>

#include "Config.h" // Dirty
#include "map.h"

class QMouseEvent;
class QDropEvent;
class KJob;
class QDragEnterEvent;

class Folder;
class File;
namespace KIO
{
class Job;
} // namespace KIO

namespace RadialMap
{
class Segment;

class Item : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)
public:
    explicit Item(QQuickItem *parent = nullptr);
    ~Item() override;
    QString path() const;
    QUrl url(File const * = nullptr) const;

    Q_SIGNAL void validChanged();
    Q_INVOKABLE bool isValid() const
    {
        return m_tree != nullptr;
    }
    friend class Label; // FIXME badness

    void paint(QPainter *painter) override;

    Q_INVOKABLE void saveSVG();

public Q_SLOTS:
    void zoomIn();
    void zoomOut();
    void create(Folder *);
    void invalidate();
    void refresh(Dirty filth);

private Q_SLOTS:
    void resizeTimeout();
    void sendFakeMouseEvent();
    void deleteJobFinished(KJob *);
    void createFromCache(Folder *);

Q_SIGNALS:
    void activated(const QUrl &);
    void invalidated(const QUrl &);
    void folderCreated(Folder *);
    void mouseHover(const QString &);
    void giveMeTreeFor(const QUrl &);
    void rescanRequested(const QUrl &);

protected:
    bool event(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

protected:
    const Segment *segmentAt(QPointF position) const; // FIXME const reference for a library others can use
    const Segment *rootSegment() const;
    const Segment *focusSegment() const;

private:
    void paintExplodedLabels(QPainter &) const;

    const Folder *m_tree = nullptr;
    const Segment *m_focus = nullptr;
    QPointF m_offset;
    QTimer m_timer;
    Map m_map;
    Segment *m_rootSegment = nullptr;
    const Segment *m_toBeDeleted = nullptr;
    QLabel m_tooltip;
};

} // namespace RadialMap
