/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
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

#include "widget.h"

#include "part/Config.h"
#include "part/fileTree.h"
#include "radialMap.h" //constants
#include "map.h"

#include <KCursor>        //ctor
#include <KUrl>

#include <QtGui/QApplication>   //sendEvent
#include <QtGui/QBitmap>        //ctor - finding cursor size
#include <QtGui/QCursor>        //slotPostMouseEvent()
#include <QtCore/QTimer>         //member
#include <QtGui/QWidget>


RadialMap::Widget::Widget(QWidget *parent, bool isSummary)
        : QWidget(parent)
        , m_tree(0)
        , m_focus(0)
        , m_map(isSummary)
        , m_rootSegment(0) //TODO we don't delete it, *shrug*
        , m_isSummary(isSummary)
        , m_toBeDeleted(0)
{
    setAcceptDrops(true);
    setMinimumSize(350, 250);

    connect(this, SIGNAL(created(const Folder*)), SLOT(sendFakeMouseEvent()));
    connect(this, SIGNAL(created(const Folder*)), SLOT(update()));
    connect(&m_timer, SIGNAL(timeout()), SLOT(resizeTimeout()));
}

RadialMap::Widget::~Widget()
{
    delete m_rootSegment;
}


QString
RadialMap::Widget::path() const
{
    return m_tree->fullPath();
}

KUrl
RadialMap::Widget::url(File const * const file) const
{
    return KUrl(file ? file->fullPath() : m_tree->fullPath());
}

void
RadialMap::Widget::invalidate()
{
    if (isValid())
    {
        //**** have to check that only way to invalidate is this function frankly
        //**** otherwise you may get bugs..

        //disable mouse tracking
        setMouseTracking(false);

        //ensure this class won't think we have a map still
        m_tree  = 0;
        m_focus = 0;

        delete m_rootSegment;
        m_rootSegment = 0;

        //FIXME move this disablement thing no?
        //      it is confusing in other areas, like the whole createFromCache() thing
        m_map.invalidate();
        update();

        //tell rest of Filelight
        emit invalidated(url());
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
        m_focus = 0;
        //generate the filemap image
        m_map.make(tree);

        //this is the inner circle in the center
        m_rootSegment = new Segment(tree, 0, 16*360);

        setMouseTracking(true);
    }

    m_tree = tree;

    //tell rest of Filelight
    emit created(tree);
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
    QMouseEvent me(QEvent::MouseMove, mapFromGlobal(QCursor::pos()), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(this, &me);
}

void
RadialMap::Widget::resizeTimeout() //slot
{
    // the segments are about to erased!
    // this was a horrid bug, and proves the OO programming should be obeyed always!
    m_focus = 0;
    if (m_tree)
        m_map.make(m_tree, true);
    update();
}

void
RadialMap::Widget::refresh(int filth)
{
    //TODO consider a more direct connection

    if (!m_map.isNull())
    {
        switch (filth)
        {
        case 1:
            m_focus=0;
            m_map.make(m_tree, true); //true means refresh only
            break;

        case 2:
            m_map.paint(true); //antialiased painting
            break;

        case 3:
            m_map.colorise(); //FALL THROUGH!
        case 4:
            m_map.paint();

        default:
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
        m_focus = 0;
        m_map.make(m_tree);
        Config::defaultRingDepth = m_map.m_visibleDepth;
        update();
    }
}

void
RadialMap::Widget::zoomOut() //slot
{
    m_focus = 0;
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

#include "widget.moc"
