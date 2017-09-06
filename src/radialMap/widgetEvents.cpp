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

#include "fileTree.h"
#include "Config.h"
#include "radialMap.h"   //class Segment
#include "widget.h"

#include <KCursor>     //::mouseMoveEvent()
#include <KJob>
#include <KIO/Job>     //::mousePressEvent()
#include <KIO//DeleteJob>
#include <KIO/JobUiDelegate>
#include <KMessageBox> //::mousePressEvent()
#include <QMenu>  //::mousePressEvent()
#include <KRun>        //::mousePressEvent()
#include <KToolInvocation>
#include <KFormat>
#include <QUrl>
#include <KLocalizedString>

#include <QApplication> //QApplication::setOverrideCursor()
#include <QClipboard>
#include <QPainter>
#include <QTimer>      //::resizeEvent()
#include <QDropEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QToolTip>
#include <QMimeData>
#include <KUrlMimeData>

#include <cmath>         //::segmentAt()

void RadialMap::Widget::resizeEvent(QResizeEvent*)
{
    if (m_map.resize(rect()))
        m_timer.setSingleShot(true);
    m_timer.start(500); //will cause signature to rebuild for new size

    //always do these as they need to be initialised on creation
    m_offset.rx() = (width() - m_map.width()) / 2;
    m_offset.ry() = (height() - m_map.height()) / 2;
}

void RadialMap::Widget::paintEvent(QPaintEvent*)
{
    QPainter paint;
    paint.begin(this);

    if (!m_map.isNull())
        paint.drawPixmap(m_offset, m_map.pixmap());
    else
    {
        paint.drawText(rect(), 0, i18nc("We messed up, the user needs to initiate a rescan.", "Internal representation is invalid,\nplease rescan."));
        return;
    }

    //exploded labels
    if (!m_map.isNull() && !m_timer.isActive())
    {
        if (Config::antialias) {
            paint.setRenderHint(QPainter::Antialiasing);
            //make lines appear on pixel boundaries
            paint.translate(0.5, 0.5);
        }
        paintExplodedLabels(paint);
    }
}

const RadialMap::Segment* RadialMap::Widget::segmentAt(QPoint &e) const
{
    //determine which segment QPoint e is above

    e -= m_offset;

    if (!m_map.m_signature)
        return nullptr;

    if (e.x() <= m_map.width() && e.y() <= m_map.height())
    {
        //transform to cartesian coords
        e.rx() -= m_map.width() / 2; //should be an int
        e.ry()  = m_map.height() / 2 - e.y();

        double length = hypot(e.x(), e.y());

        if (length >= m_map.m_innerRadius) //not hovering over inner circle
        {
            uint depth  = ((int)length - m_map.m_innerRadius) / m_map.m_ringBreadth;

            if (depth <= m_map.m_visibleDepth) //**** do earlier since you can //** check not outside of range
            {
                //vector calculation, reduces to simple trigonometry
                //cos angle = (aibi + ajbj) / albl
                //ai = x, bi=1, aj=y, bj=0
                //cos angle = x / (length)

                uint a  = (uint)(acos((double)e.x() / length) * 916.736);  //916.7324722 = #radians in circle * 16

                //acos only understands 0-180 degrees
                if (e.y() < 0) a = 5760 - a;

                for (Segment *segment : m_map.m_signature[depth]) {
                    if (segment->intersects(a))
                        return segment;
                }
            }
        }
        else return m_rootSegment; //hovering over inner circle
    }

    return nullptr;
}

void RadialMap::Widget::mouseMoveEvent(QMouseEvent *e)
{
    //set m_focus to what we hover over, update UI if it's a new segment

    Segment const * const oldFocus = m_focus;
    QPoint p = e->pos();

    m_focus = segmentAt(p); //NOTE p is passed by non-const reference

    if (m_focus)
    {
        m_tooltip.move(e->globalX() + 20, e->globalY() + 20);
        if (m_focus != oldFocus) //if not same as last time
        {
            setCursor(Qt::PointingHandCursor);


            QString string = m_focus->file()->fullPath(m_tree)
                + QLatin1Char('\n')
                + m_focus->file()->humanReadableSize();

            if (m_focus->file()->isFolder()) {
                int files = static_cast<const Folder*>(m_focus->file())->children();
                const uint percent = uint((100 * files) / (double)m_tree->children());
                string += QLatin1Char('\n');
                string += i18np("File: %1", "Files: %1", files);


                if (percent > 0) string += QString(QLatin1String(" (%1%)")).arg(percent);
            }

            const QUrl url = Widget::url(m_focus->file());
            if (m_focus == m_rootSegment && url != KIO::upUrl(url)) {
                string += i18n("\nClick to go up to parent directory");
            }

            // Calculate a semi-sane size for the tooltip
            QFontMetrics fontMetrics(font());
            int tooltipWidth = 0;
            int tooltipHeight = 0;
            for (const QString &part : string.split(QLatin1Char('\n'))) {
                tooltipHeight += fontMetrics.height();
                tooltipWidth = qMax(tooltipWidth, fontMetrics.width(part));
            }
            // Limit it to the window size, probably should find something better
            tooltipWidth = qMin(tooltipWidth, window()->width());
            tooltipWidth += 10;
            tooltipHeight += 10;
            m_tooltip.resize(tooltipWidth, tooltipHeight);
            m_tooltip.setText(string);
            m_tooltip.show();

            emit mouseHover(m_focus->file()->fullPath());
            update();
        }
    }
    else if (oldFocus && oldFocus->file() != m_tree)
    {
        m_tooltip.hide();
        unsetCursor();
        update();

        emit mouseHover(QString());
    }
}

void RadialMap::Widget::enterEvent(QEvent *)
{
    if (!m_focus) return;

    setCursor(Qt::PointingHandCursor);
    emit mouseHover(m_focus->file()->fullPath());
    update();
}

void RadialMap::Widget::leaveEvent(QEvent *)
{
    m_tooltip.hide();
}

void RadialMap::Widget::mousePressEvent(QMouseEvent *e)
{
    if (!isEnabled())
        return;

    //m_focus is set correctly (I've been strict, I assure you it is correct!)

    if (!m_focus || m_focus->isFake()) {
        return;
    }

    const QUrl url   = Widget::url(m_focus->file());
    const bool isDir = m_focus->file()->isFolder();

    // Open file
    if (e->button() == Qt::MidButton || (e->button() == Qt::LeftButton && !isDir)) {
        new KRun(url, this, true);

        return;
    }

    if (e->button() == Qt::LeftButton) {
        if (m_focus->file() != m_tree) {
            emit activated(url); //activate first, this will cause UI to prepare itself
            createFromCache((Folder *)m_focus->file());
        } else if (KIO::upUrl(url) != url) {
            emit giveMeTreeFor(KIO::upUrl(url));
        }

        return;
    }

    if (e->button() != Qt::RightButton) {
        // Ignore other mouse buttons
        return;
    }

    // Actions in the right click menu
    QAction* openFileManager = nullptr;
    QAction* openTerminal = nullptr;
    QAction* centerMap = nullptr;
    QAction* openFile = nullptr;
    QAction* copyClipboard = nullptr;
    QAction* deleteItem = nullptr;

    QMenu popup;
    popup.setTitle(m_focus->file()->fullPath(m_tree));

    if (isDir) {
        openFileManager = popup.addAction(QIcon::fromTheme(QLatin1String("system-file-manager")), i18n("Open &File Manager Here"));

        if (url.scheme() == QLatin1String("file")) {
            openTerminal = popup.addAction(QIcon::fromTheme(QLatin1String( "utilities-terminal" )), i18n("Open &Terminal Here"));
        }

        if (m_focus->file() != m_tree) {
            popup.addSeparator();
            centerMap = popup.addAction(QIcon::fromTheme(QLatin1String( "zoom-in" )), i18n("&Center Map Here"));
        }
    } else {
        openFile = popup.addAction(QIcon::fromTheme(QLatin1String("document-open")), i18nc("Scan/open the path of the selected element", "&Open"));
    }

    popup.addSeparator();
    copyClipboard = popup.addAction(QIcon::fromTheme(QLatin1String( "edit-copy" )), i18n("&Copy to clipboard"));

    if (m_focus->file() != m_tree) {
        popup.addSeparator();
        deleteItem = popup.addAction(QIcon::fromTheme(QLatin1String( "edit-delete" )), i18n("&Delete"));
    }

    QAction* clicked = popup.exec(e->globalPos(), nullptr);

    if (openFileManager && clicked == openFileManager) {
        KRun::runUrl(url, QLatin1String( "inode/directory" ), this);
    } else if (openTerminal && clicked == openTerminal) {
        KToolInvocation::invokeTerminal(QString(),url.path());
    } else if (centerMap && clicked == centerMap) {
        emit activated(url); //activate first, this will cause UI to prepare itself
        createFromCache((Folder *)m_focus->file());
    } else if (openFile && clicked == openFile) {
        new KRun(url, this, true);
    } else if (clicked == copyClipboard) {
        QMimeData* mimedata = new QMimeData();
        mimedata->setUrls(QList<QUrl>() << url);
        QApplication::clipboard()->setMimeData(mimedata , QClipboard::Clipboard);
    } else if (clicked == deleteItem && m_focus->file() != m_tree) {
        m_toBeDeleted = m_focus;
        const QUrl url = Widget::url(m_toBeDeleted->file());
        const QString message = m_toBeDeleted->file()->isFolder()
                ? i18n("<qt>The folder at <i>'%1'</i> will be <b>recursively</b> and <b>permanently</b> deleted.</qt>", url.toString())
                : i18n("<qt><i>'%1'</i> will be <b>permanently</b> deleted.</qt>", url.toString());
        const int userIntention = KMessageBox::warningContinueCancel(
                    this, message,
                    QString(), KGuiItem(i18n("&Delete"), QLatin1String("edit-delete")));

        if (userIntention == KMessageBox::Continue) {
            KIO::Job *job = KIO::del(url);
            connect(job, &KJob::finished, this, &RadialMap::Widget::deleteJobFinished);
            QApplication::setOverrideCursor(Qt::BusyCursor);
            setEnabled(false);
        }
    } else {
        //ensure m_focus is set for new mouse position
        sendFakeMouseEvent();
    }
}

void RadialMap::Widget::deleteJobFinished(KJob *job)
{
    QApplication::restoreOverrideCursor();
    setEnabled(true);
    if (!job->error() && m_toBeDeleted) {
        m_toBeDeleted->file()->parent()->remove(m_toBeDeleted->file());
        delete m_toBeDeleted->file();
        m_toBeDeleted = nullptr;
        m_focus = nullptr;
        m_map.make(m_tree, true);
        repaint();
    } else
        KMessageBox::error(this, job->errorString(), i18n("Error while deleting"));
}

void RadialMap::Widget::dropEvent(QDropEvent *e)
{
    QList<QUrl> uriList = KUrlMimeData::urlsFromMimeData(e->mimeData());
    if (!uriList.isEmpty())
        emit giveMeTreeFor(uriList.first());
}

void RadialMap::Widget::dragEnterEvent(QDragEnterEvent *e)
{
    QList<QUrl> uriList = KUrlMimeData::urlsFromMimeData(e->mimeData());
    e->setAccepted(!uriList.isEmpty());
}

void RadialMap::Widget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::ApplicationPaletteChange ||
        e->type() == QEvent::PaletteChange)
        m_map.paint();
}
