/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "widget.h"

#include "fileTree.h"
#include "radialMap.h" //constants

#include <KCursor>        //ctor

#include <QApplication>   //sendEvent
#include <QBitmap>        //ctor - finding cursor size
#include <QCursor>        //slotPostMouseEvent()
#include <QDebug>
#include <QTimer>         //member


RadialMap::Widget::Widget(QWidget *parent, bool isSummary)
        : QWidget(parent)
        , m_tree(nullptr)
        , m_focus(nullptr)
        , m_map(isSummary)
        , m_rootSegment(nullptr) //TODO we don't delete it, *shrug*
        , m_isSummary(isSummary)
        , m_toBeDeleted(nullptr)
{
    setAcceptDrops(true);
    setMinimumSize(350, 250);

    connect(this, &Widget::folderCreated, this, &Widget::sendFakeMouseEvent);
    connect(&m_timer, &QTimer::timeout, this, &Widget::resizeTimeout);
    m_tooltip.setFrameShape(QFrame::StyledPanel);
    m_tooltip.setWindowFlags(Qt::ToolTip | Qt::WindowTransparentForInput);
    m_map.m_dpr = devicePixelRatioF();
}

RadialMap::Widget::~Widget()
{
    delete m_rootSegment;
}


QString RadialMap::Widget::path() const
{
    return m_tree->displayPath();
}

QUrl RadialMap::Widget::url(File const * const file) const
{
    return file ? file->url() : m_tree->url();
}

void RadialMap::Widget::invalidate()
{
    if (isValid())
    {
        //**** have to check that only way to invalidate is this function frankly
        //**** otherwise you may get bugs..

        //disable mouse tracking
        setMouseTracking(false);

        // Get this before reseting m_tree below
        QUrl invalidatedUrl(url());

        //ensure this class won't think we have a map still
        m_tree  = nullptr;
        m_focus = nullptr;

        delete m_rootSegment;
        m_rootSegment = nullptr;

        //FIXME move this disablement thing no?
        //      it is confusing in other areas, like the whole createFromCache() thing
        m_map.invalidate();
        update();

        //tell rest of Filelight
        Q_EMIT invalidated(invalidatedUrl);
    }
}

void
RadialMap::Widget::create(const Folder *tree)
{
    //it is not the responsibility of create() to invalidate first
    //skip invalidation at your own risk

    //FIXME make it the responsibility of create to invalidate first

    if (tree)
    {
        m_focus = nullptr;
        //generate the filemap image
        m_map.make(tree);

        //this is the inner circle in the center
        m_rootSegment = new Segment(tree, 0, 16*360);

        setMouseTracking(true);
    }

    m_tree = tree;

    //tell rest of Filelight
    Q_EMIT folderCreated(tree);
}

void
RadialMap::Widget::createFromCache(const Folder *tree)
{
    //no scan was necessary, use cached tree, however we MUST still emit invalidate
    invalidate();
    create(tree);
}

void
RadialMap::Widget::sendFakeMouseEvent() //slot
{
    // If we're not the focused window (or on another desktop), don't pop up our tooltip
    if (!qApp->focusWindow()) {
        return;
    }

    QMouseEvent me(QEvent::MouseMove, mapFromGlobal(QCursor::pos()), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(this, &me);
    update();
}

void
RadialMap::Widget::resizeTimeout() //slot
{
    // the segments are about to erased!
    // this was a horrid bug, and proves the OO programming should be obeyed always!
    m_focus = nullptr;
    if (m_tree)
        m_map.make(m_tree, true);
    update();
}

void
RadialMap::Widget::refresh(const Dirty filth)
{
    //TODO consider a more direct connection

    if (!m_map.isNull())
    {
        switch (filth)
        {
        case Dirty::Layout:
            m_focus = nullptr;
            m_map.make(m_tree, true); //true means refresh only
            break;

        case Dirty::AntiAliasing:
            m_map.paint();
            break;

        case Dirty::Colors:
            m_map.colorise();
            m_map.paint();
            break;

        // At the time of writing only used by the exploded labels
        // which is redrawn with each paintEvent(), so just need an update()
        case Dirty::Font:
            break;

        default:
            qWarning() << "Unhandled filth type" << int(filth);
            break;
        }

        update();
    }
}

void
RadialMap::Widget::zoomIn() //slot
{
    if (m_map.m_visibleDepth > MIN_RING_DEPTH)
    {
        --m_map.m_visibleDepth;
        m_focus = nullptr;
        m_map.make(m_tree);
        Config::defaultRingDepth = m_map.m_visibleDepth;
        update();
    }
}

void
RadialMap::Widget::zoomOut() //slot
{
    m_focus = nullptr;
    ++m_map.m_visibleDepth;
    m_map.make(m_tree);
    if (m_map.m_visibleDepth > Config::defaultRingDepth)
        Config::defaultRingDepth = m_map.m_visibleDepth;
    update();
}


RadialMap::Segment::~Segment()
{
    if (isFake())
        delete m_file; //created by us in Builder::build()
}


