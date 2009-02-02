/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <sandsmark@samfundet.no>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#ifndef WIDGET_H
#define WIDGET_H

#include <KUrl>
#include <QTimer>
#include <QPixmap>
//Added by qt3to4:
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QPaintEvent>

#include "segmentTip.h"
#include "map.h"

template <class T> class Chain;
class Directory;
class File;
namespace KIO {
class Job;
}
class KUrl;

namespace RadialMap
{
class Segment;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget* = 0);
    ~Widget() {
        delete m_tip;
    }

    QString path() const;
    KUrl url(File const * const = 0) const;

    bool isValid() const {
        return m_tree != 0;
    }

    friend class Label; //FIXME badness

public slots:
    void zoomIn();
    void zoomOut();
    void create(const Directory*);
    void invalidate(const bool = true);
    void refresh(int);

private slots:
    void resizeTimeout();
    void sendFakeMouseEvent();
    void deleteJobFinished(KIO::Job*);
    void createFromCache(const Directory*);

signals:
    void activated(const KUrl&);
    void invalidated(const KUrl&);
    void created(const Directory*);
    void mouseHover(const QString&);
    void giveMeTreeFor(const KUrl&);

protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void resizeEvent(QResizeEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dropEvent(QDropEvent*);

protected:
    const Segment *segmentAt(QPoint&) const; //FIXME const reference for a library others can use
    const Segment *rootSegment() const {
        return m_rootSegment;    ///never == 0
    }
    const Segment *focusSegment() const {
        return m_focus;    ///0 == nothing in focus
    }

private:
    void paintExplodedLabels(QPainter&) const;

    const Directory *m_tree;
    const Segment   *m_focus;
    QPoint           m_offset;
    QTimer           m_timer;
    Map              m_map;
    SegmentTip       *m_tip;
    Segment          *m_rootSegment;
};
}

#endif
