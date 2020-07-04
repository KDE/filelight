/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef WIDGET_H
#define WIDGET_H

#include <KJob>
#include <QUrl>

#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWidget>
#include <QTimer>

#include "map.h"
#include "Config.h" // Dirty

class Folder;
class File;
namespace KIO {
class Job;
}

namespace RadialMap
{
class Segment;

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget* = nullptr, bool = false);
    ~Widget() override;
    QString path() const;
    QUrl url(File const * const = nullptr) const;

    bool isValid() const {
        return m_tree != nullptr;
    }

    bool isSummary() const {
        return m_isSummary;
    }

    friend class Label; //FIXME badness

    void saveSvg(const QString &path) { m_map.saveSvg(path); }

public Q_SLOTS:
    void zoomIn();
    void zoomOut();
    void create(const Folder*);
    void invalidate();
    void refresh(const Dirty filth);

private Q_SLOTS:
    void resizeTimeout();
    void sendFakeMouseEvent();
    void deleteJobFinished(KJob*);
    void createFromCache(const Folder*);

Q_SIGNALS:
    void activated(const QUrl&);
    void invalidated(const QUrl&);
    void folderCreated(const Folder*);
    void mouseHover(const QString&);
    void giveMeTreeFor(const QUrl&);
    void rescanRequested(const QUrl&);

protected:
    void changeEvent(QEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void enterEvent(QEvent*) override;
    void leaveEvent(QEvent*) override;

protected:
    const Segment *segmentAt(QPointF position) const; //FIXME const reference for a library others can use
    const Segment *rootSegment() const {
        return m_rootSegment;    ///never == 0
    }
    const Segment *focusSegment() const {
        return m_focus;    ///0 == nothing in focus
    }

private:
    void paintExplodedLabels(QPainter&) const;

    const Folder *m_tree;
    const Segment   *m_focus;
    QPointF           m_offset;
    QTimer           m_timer;
    Map              m_map;
    Segment          *m_rootSegment;
    const bool       m_isSummary;
    const Segment    *m_toBeDeleted;
    QLabel           m_tooltip;
};
}

#endif
