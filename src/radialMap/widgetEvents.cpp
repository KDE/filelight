/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "fileTree.h"
#include "Config.h"
#include "radialMap.h"   //class Segment
#include "widget.h"

#include <KCursor>     //::mouseMoveEvent()
#include <KJob>
#include <KIO/Job>     //::mousePressEvent()
#include <KIO//DeleteJob>
#include <KMessageBox> //::mousePressEvent()
#include <QMenu>  //::mousePressEvent()
#include <KToolInvocation>
#include <QUrl>
#include <KIO/JobUiDelegate>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <kio_version.h>

#include <QApplication> //QApplication::setOverrideCursor()
#include <QClipboard>
#include <QPainter>
#include <QTimer>      //::resizeEvent()
#include <QDropEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <KUrlMimeData>
#include <QWindow>
#include <QScreen>

#include <cmath>         //::segmentAt()

#include <KIO/OpenUrlJob>

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
    else {
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

const RadialMap::Segment* RadialMap::Widget::segmentAt(QPointF e) const
{
    //determine which segment QPointF e is above

    e -= m_offset;

    if (m_map.m_signature.isEmpty())
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
    m_focus = segmentAt(e->pos());

    if (!m_focus) {
        if (oldFocus && oldFocus->file() != m_tree) {
            m_tooltip.hide();
            unsetCursor();
            update();

            Q_EMIT mouseHover(QString());
        }

        return;
    }

    const QRectF screenRect = window()->windowHandle()->screen()->availableGeometry();

    QPoint tooltipPosition = e->globalPos() + QPoint(20, 20);
    QRectF tooltipRect(tooltipPosition, m_tooltip.size());

    // Same content as before
    if (m_focus == oldFocus) {
        if (tooltipRect.right() > screenRect.right()) {
            tooltipPosition.setX(screenRect.x() + screenRect.width() - m_tooltip.width());
        }
        if (tooltipRect.bottom() > screenRect.bottom()) {
            tooltipPosition.setY(screenRect.y() + screenRect.height() - m_tooltip.height());
        }
        m_tooltip.move(tooltipPosition);
        return;
    }

    setCursor(Qt::PointingHandCursor);

    QString string;

    if (isSummary()) {
        if (strcmp("used", m_focus->file()->name8Bit()) == 0) {
            string = i18nc("Tooltip of used space on the partition, %1 is path, %2 is size",
                    "%1\nUsed: %2",
                    m_focus->file()->parent()->displayPath(),
                    m_focus->file()->humanReadableSize());
        } else if (strcmp("free", m_focus->file()->name8Bit()) == 0) {
            string = i18nc("Tooltip of free space on the partition, %1 is path, %2 is size",
                    "%1\nFree: %2",
                    m_focus->file()->parent()->displayPath(),
                    m_focus->file()->humanReadableSize());
        } else {
            string = i18nc("Tooltip of file/folder, %1 is path, %2 is size",
                    "%1\n%2",
                    m_focus->file()->displayPath(),
                    m_focus->file()->humanReadableSize());
        }
    } else {
        string = i18nc("Tooltip of file/folder, %1 is path, %2 is size",
                "%1\n%2",
                m_focus->file()->displayPath(),
                m_focus->file()->humanReadableSize());

        if (m_focus->file()->isFolder()) {
            int files = static_cast<const Folder*>(m_focus->file())->children();
            const uint percent = uint((100 * files) / (double)m_tree->children());

            string += QLatin1Char('\n');
            if (percent > 0) {
                string += i18ncp("Tooltip of folder, %1 is number of files",
                        "%1 File (%2%)", "%1 Files (%2%)",
                        files, percent);
            } else {
                string += i18ncp("Tooltip of folder, %1 is number of files",
                        "%1 File", "%1 Files",
                        files);
            }
        }

        const QUrl url = Widget::url(m_focus->file());
        if (m_focus == m_rootSegment && url != KIO::upUrl(url)) {
            string += i18n("\nClick to go up to parent directory");
        }
    }

    // Calculate a semi-sane size for the tooltip
    QFontMetrics fontMetrics(font());
    int tooltipWidth = 0;
    int tooltipHeight = 0;
    for (const QString &part : string.split(QLatin1Char('\n'))) {
        tooltipHeight += fontMetrics.height();
        tooltipWidth = qMax(tooltipWidth, fontMetrics.boundingRect(part).width());
    }
    tooltipWidth += 10;
    tooltipHeight += 10;

    m_tooltip.resize(tooltipWidth, tooltipHeight);
    m_tooltip.setText(string);

    // Make sure we're visible on screen
    tooltipRect.setSize(QSize(tooltipWidth, tooltipHeight));
    if (tooltipRect.right() > screenRect.right()) {
        tooltipPosition.setX(screenRect.x() + screenRect.width() - m_tooltip.width());
    }
    if (tooltipRect.bottom() > screenRect.bottom()) {
        tooltipPosition.setY(screenRect.y() + screenRect.height() - m_tooltip.height());
    }
    m_tooltip.move(tooltipPosition);

    m_tooltip.show();

    Q_EMIT mouseHover(m_focus->file()->displayPath());
    update();
}

void RadialMap::Widget::enterEvent(QEvent *)
{
    if (!m_focus) return;

    setCursor(Qt::PointingHandCursor);
    Q_EMIT mouseHover(m_focus->file()->displayPath());
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
    if (e->button() == Qt::MiddleButton || (e->button() == Qt::LeftButton && !isDir)) {
        auto *job = new KIO::OpenUrlJob(QUrl(url));
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        job->start();
        return;
    }

    if (e->button() == Qt::LeftButton) {
        if (m_focus->file() != m_tree) {
            Q_EMIT activated(url); //activate first, this will cause UI to prepare itself
            createFromCache((Folder *)m_focus->file());
        } else if (KIO::upUrl(url) != url) {
            Q_EMIT giveMeTreeFor(KIO::upUrl(url));
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
    QAction* doNotScanItem = nullptr;
    QAction* rescanAction = nullptr;

    QMenu popup;
    popup.setTitle(m_focus->file()->displayPath(m_tree));

    if (isDir) {
        openFileManager = popup.addAction(QIcon::fromTheme(QStringLiteral("system-file-manager")), i18n("Open &File Manager Here"));

        if (url.scheme() == QLatin1String("file")) {
            openTerminal = popup.addAction(QIcon::fromTheme(QStringLiteral( "utilities-terminal" )), i18n("Open &Terminal Here"));
        }

        if (m_focus->file() != m_tree) {
            popup.addSeparator();
            centerMap = popup.addAction(QIcon::fromTheme(QStringLiteral( "zoom-in" )), i18n("&Center Map Here"));
        }
        
        popup.addSeparator();
        doNotScanItem = popup.addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Add to Do &Not Scan List"));
        rescanAction = popup.addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("&Rescan"));
    } else {
        openFile = popup.addAction(QIcon::fromTheme(QStringLiteral("document-open")), i18nc("Scan/open the path of the selected element", "&Open"));
    }

    popup.addSeparator();
    copyClipboard = popup.addAction(QIcon::fromTheme(QStringLiteral( "edit-copy" )), i18n("&Copy to clipboard"));

    if (m_focus->file() != m_tree) {
        popup.addSeparator();
        deleteItem = popup.addAction(QIcon::fromTheme(QStringLiteral( "edit-delete" )), i18n("&Delete"));
    }

    QAction* clicked = popup.exec(e->globalPos(), nullptr);

    if (openFileManager && clicked == openFileManager) {
            KIO::OpenUrlJob *job = new KIO::OpenUrlJob(url, QStringLiteral("inode/directory"), this);
            job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window()));
            job->start();
    } else if (rescanAction && clicked == rescanAction) {
        Q_EMIT rescanRequested(url);
    } else if (openTerminal && clicked == openTerminal) {
        KToolInvocation::invokeTerminal(QString(), QStringList(), url.path());
    } else if (centerMap && clicked == centerMap) {
        Q_EMIT activated(url); //activate first, this will cause UI to prepare itself
        createFromCache((Folder *)m_focus->file());
    } else if (doNotScanItem && clicked == doNotScanItem) {
        if (!Config::skipList.contains(Widget::url(m_focus->file()).toLocalFile())) {
            Config::skipList.append(Widget::url(m_focus->file()).toLocalFile());
            Config::write();
        }
    } else if (openFile && clicked == openFile) {
        auto *job = new KIO::OpenUrlJob(url);
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        job->start();
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
                    QString(), KGuiItem(i18n("&Delete"), QStringLiteral("edit-delete")));

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
        m_toBeDeleted = nullptr;
        m_focus = nullptr;
        m_map.make(m_tree, true);
        update();
    } else
        KMessageBox::error(this, job->errorString(), i18n("Error while deleting"));
}

void RadialMap::Widget::dropEvent(QDropEvent *e)
{
    QList<QUrl> uriList = KUrlMimeData::urlsFromMimeData(e->mimeData());
    if (!uriList.isEmpty())
        Q_EMIT giveMeTreeFor(uriList.first());
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
