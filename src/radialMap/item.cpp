/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "item.h"

#include <cmath> //::segmentAt()
#include <utility>

#include <QApplication> //sendEvent
#include <QClipboard>
#include <QCursor> //slotPostMouseEvent()
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QMenu> //::mousePressEvent()
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QQuickWindow>
#include <QResizeEvent>
#include <QScreen>
#include <QWindow>

#include <KIO/DeleteJob>
#include <KIO/Job> //::mousePressEvent()
#include <KIO/JobUiDelegate>
#include <KIO/OpenUrlJob>
#include <KJob>
#include <KLocalizedString>
#include <KMessageBox> //::mousePressEvent()
#include <KTerminalLauncherJob>
#include <KUrlMimeData>

#include "fileTree.h"
#include "filelight_debug.h"
#include "labels.h"

RadialMap::Item::Item(QQuickItem *parent)
    : QQuickPaintedItem(parent)

{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setFlag(QQuickItem::ItemAcceptsDrops, true);

    connect(this, &Item::folderCreated, this, &Item::sendFakeMouseEvent);
    connect(&m_timer, &QTimer::timeout, this, &Item::resizeTimeout);
    m_tooltip.setFrameShape(QFrame::StyledPanel);
    m_tooltip.setWindowFlags(Qt::ToolTip | Qt::WindowTransparentForInput);

    /// ///////////

    connect(this, &Item::widthChanged, this, [this]() {
        if (m_map.resize(QRectF(x(), y(), width(), height()))) {
            m_timer.setSingleShot(true);
        }
        m_timer.start(500); // will cause signature to rebuild for new size

        // always do these as they need to be initialised on creation
        m_offset.rx() = (width() - m_map.width()) / 2;
        m_offset.ry() = (height() - m_map.height()) / 2;
    });

    connect(this, &Item::heightChanged, this, [this]() {
        if (m_map.resize(QRectF(x(), y(), width(), height()))) {
            m_timer.setSingleShot(true);
        }
        m_timer.start(500); // will cause signature to rebuild for new size

        // always do these as they need to be initialised on creation
        m_offset.rx() = (width() - m_map.width()) / 2;
        m_offset.ry() = (height() - m_map.height()) / 2;
    });
}

QString RadialMap::Item::path() const
{
    return m_tree->displayPath();
}

QUrl RadialMap::Item::url(const std::shared_ptr<File> &file) const
{
    return file ? file->url() : m_tree->url();
}

void RadialMap::Item::invalidate()
{
    if (isValid()) {
        //**** have to check that only way to invalidate is this function frankly
        //**** otherwise you may get bugs..

        // disable mouse tracking
        //  setMouseTracking(false);

        // Get this before reseting m_tree below
        QUrl invalidatedUrl(url());

        // ensure this class won't think we have a map still
        m_tree = nullptr;
        Q_EMIT validChanged();
        m_focus = nullptr;

        m_rootSegment = nullptr;

        // FIXME move this disablement thing no?
        //       it is confusing in other areas, like the whole createFromCache() thing
        m_map.invalidate();
        update();

        // tell rest of Filelight
        Q_EMIT invalidated(invalidatedUrl);
    }
}

void RadialMap::Item::create(std::shared_ptr<Folder> tree)
{
    // it is not the responsibility of create() to invalidate first
    // skip invalidation at your own risk

    // FIXME make it the responsibility of create to invalidate first

    if (tree) {
        m_focus = nullptr;
        // generate the filemap image
        m_map.make(tree);

        // this is the inner circle in the center
        m_rootSegment = std::make_unique<Segment>(tree, 0, 16 * 360);

        // setMouseTracking(true);
    }

    m_tree = tree;
    Q_EMIT validChanged();

    // tell rest of Filelight
    qCDebug(FILELIGHT_LOG) << "emitting folder created" << tree.get();
    Q_EMIT folderCreated();
    Q_EMIT treeChildrenChanged();
}

void RadialMap::Item::mousePressEvent(QMouseEvent *e)
{
    if (!isEnabled()) {
        return;
    }

    // m_focus is set correctly (I've been strict, I assure you it is correct!)

    if (!m_focus || m_focus->isFake()) {
        return;
    }

    const QUrl url = Item::url(m_focus->file());
    const bool isDir = m_focus->file()->isFolder();

    // Open file
    if (e->button() == Qt::MiddleButton || (e->button() == Qt::LeftButton && !isDir)) {
        auto *job = new KIO::OpenUrlJob(QUrl(url));
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
        job->start();
        return;
    }

    if (e->button() == Qt::LeftButton) {
        if (m_focus->file() != m_tree) {
            Q_EMIT activated(url); // activate first, this will cause UI to prepare itself
            createFromCache(std::dynamic_pointer_cast<Folder>(m_focus->file()));
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
    QAction *openFileManager = nullptr;
    QAction *openTerminal = nullptr;
    QAction *centerMap = nullptr;
    QAction *openFile = nullptr;
    QAction *copyClipboard = nullptr;
    QAction *deleteItem = nullptr;
    QAction *doNotScanItem = nullptr;
    QAction *rescanAction = nullptr;

    QMenu popup;
    popup.setTitle(m_focus->file()->displayPath(m_tree));

    if (isDir) {
        openFileManager = popup.addAction(QIcon::fromTheme(QStringLiteral("system-file-manager")), i18n("Open &File Manager Here"));

        if (url.scheme() == QLatin1String("file")) {
            openTerminal = popup.addAction(QIcon::fromTheme(QStringLiteral("utilities-terminal")), i18n("Open &Terminal Here"));
        }

        if (m_focus->file() != m_tree) {
            popup.addSeparator();
            centerMap = popup.addAction(QIcon::fromTheme(QStringLiteral("zoom-in")), i18n("&Center Map Here"));
        }

        popup.addSeparator();
        doNotScanItem = popup.addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Add to Do &Not Scan List"));
        rescanAction = popup.addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("&Rescan"));
    } else {
        openFile = popup.addAction(QIcon::fromTheme(QStringLiteral("document-open")), i18nc("Scan/open the path of the selected element", "&Open"));
    }

    popup.addSeparator();
    copyClipboard = popup.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("&Copy to clipboard"));

    if (m_focus->file() != m_tree) {
        popup.addSeparator();
        deleteItem = popup.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("&Delete"));
    }

    QAction *clicked = popup.exec(e->globalPos(), nullptr);

    if (openFileManager && clicked == openFileManager) {
        auto *job = new KIO::OpenUrlJob(url, QStringLiteral("inode/directory"), nullptr);
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
        job->start();
    } else if (rescanAction && clicked == rescanAction) {
        Q_EMIT rescanRequested(url);
    } else if (openTerminal && clicked == openTerminal) {
        auto *job = new KTerminalLauncherJob(QString(), this);
        job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
        job->setWorkingDirectory(url.path());
        job->start();
    } else if (centerMap && clicked == centerMap) {
        Q_EMIT activated(url); // activate first, this will cause UI to prepare itself
        createFromCache(std::dynamic_pointer_cast<Folder>(m_focus->file()));
    } else if (doNotScanItem && clicked == doNotScanItem) {
        if (!Config::skipList.contains(Item::url(m_focus->file()).toLocalFile())) {
            Config::skipList.append(Item::url(m_focus->file()).toLocalFile());
            Config::write();
        }
    } else if (openFile && clicked == openFile) {
        auto *job = new KIO::OpenUrlJob(url);
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
        job->start();
    } else if (clicked == copyClipboard) {
        auto *mimedata = new QMimeData();
        mimedata->setUrls(QList<QUrl>() << url);
        QApplication::clipboard()->setMimeData(mimedata, QClipboard::Clipboard);
    } else if (clicked == deleteItem && m_focus->file() != m_tree) {
        m_toBeDeleted = m_focus;
        const QUrl url = Item::url(m_toBeDeleted->file());
        const QString message = m_toBeDeleted->file()->isFolder()
            ? i18n("<qt>The folder at <i>'%1'</i> will be <b>recursively</b> and <b>permanently</b> deleted.</qt>", url.toString())
            : i18n("<qt><i>'%1'</i> will be <b>permanently</b> deleted.</qt>", url.toString());
        const int userIntention = KMessageBox::warningContinueCancel(nullptr, message, QString(), KGuiItem(i18n("&Delete"), QStringLiteral("edit-delete")));

        if (userIntention == KMessageBox::Continue) {
            KIO::Job *job = KIO::del(url);
            connect(job, &KJob::finished, this, &RadialMap::Item::deleteJobFinished);
            QApplication::setOverrideCursor(Qt::BusyCursor);
            setEnabled(false);
        }
    } else {
        // ensure m_focus is set for new mouse position
        sendFakeMouseEvent();
    }
}

void RadialMap::Item::deleteJobFinished(KJob *job)
{
    QApplication::restoreOverrideCursor();
    setEnabled(true);
    if (!job->error() && m_toBeDeleted) {
        m_toBeDeleted->file()->parent()->remove(std::dynamic_pointer_cast<Folder>(m_toBeDeleted->file()));
        m_toBeDeleted = nullptr;
        m_focus = nullptr;
        m_map.make(m_tree, true);
        update();
    } else {
        KMessageBox::error(nullptr, job->errorString(), i18n("Error while deleting"));
    }
}

void RadialMap::Item::createFromCache(const std::shared_ptr<Folder> &tree)
{
    qCDebug(FILELIGHT_LOG) << "Creating cached tree";
    // no scan was necessary, use cached tree, however we MUST still emit invalidate
    invalidate();
    create(tree);
}

void RadialMap::Item::sendFakeMouseEvent() // slot
{
    // If we're not the focused window (or on another desktop), don't pop up our tooltip
    if (!qApp->focusWindow()) {
        return;
    }

    QMouseEvent me(QEvent::MouseMove, mapFromGlobal(QCursor::pos()), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(this, &me);
    update();
}

void RadialMap::Item::resizeTimeout() // slot
{
    // the segments are about to erased!
    // this was a horrid bug, and proves the OO programming should be obeyed always!
    m_focus = nullptr;
    if (m_tree) {
        m_map.make(m_tree, true);
    }
    update();
}

void RadialMap::Item::refresh(const Dirty filth)
{
    // TODO consider a more direct connection

    if (!m_map.isNull()) {
        switch (filth) {
        case Dirty::Layout:
            m_focus = nullptr;
            m_map.make(m_tree, true); // true means refresh only
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

void RadialMap::Item::zoomIn() // slot
{
    if (m_map.m_visibleDepth > MIN_RING_DEPTH) {
        --m_map.m_visibleDepth;
        m_focus = nullptr;
        m_map.make(m_tree);
        Config::defaultRingDepth = m_map.m_visibleDepth;
        update();
    }
}

void RadialMap::Item::zoomOut() // slot
{
    m_focus = nullptr;
    ++m_map.m_visibleDepth;
    m_map.make(m_tree);
    if (m_map.m_visibleDepth > Config::defaultRingDepth) {
        Config::defaultRingDepth = m_map.m_visibleDepth;
    }
    update();
}

void RadialMap::Item::paint(QPainter *painter)
{
    m_map.m_dpr = painter->device()->devicePixelRatioF();

    if (!m_map.isNull()) {
        m_map.paint((QPaintDevice *)nullptr);
        painter->drawPixmap(m_offset, m_map.pixmap());
        // m_map.paint(painter);
    } else {
        painter->drawText(QRectF(x(), y(), width(), height()),
                          0,
                          i18nc("We messed up, the user needs to initiate a rescan.", "Internal representation is invalid,\nplease rescan."));
        return;
    }

    // exploded labels
    if (!m_map.isNull() && !m_timer.isActive()) {
        if (Config::antialias) {
            painter->setRenderHint(QPainter::Antialiasing);
            // make lines appear on pixel boundaries
            painter->translate(0.5, 0.5);
        }
        paintExplodedLabels(*painter);
    }
}

void RadialMap::Item::paintExplodedLabels(QPainter &paint) const
{
    // we are a friend of RadialMap::Map

    QVector<Label *> list;
    unsigned int startLevel = 0;

    // 1. Create list of labels  sorted in the order they will be rendered

    if (m_focus && m_focus->file() != m_tree) { // separate behavior for selected vs unselected segments
        // don't bother with files
        if (m_focus && m_focus->file() && !m_focus->file()->isFolder()) {
            return;
        }

        // find the range of levels we will be potentially drawing labels for
        // startLevel is the level above whatever m_focus is in
        for (const Folder *p = dynamic_cast<const Folder *>(m_focus->file().get()); p != m_tree.get(); ++startLevel) {
            p = p->parent();
        }

        // range=2 means 2 levels to draw labels for

        const uint start = m_focus->start();
        const uint end = m_focus->end(); // boundary angles
        const uint minAngle = int(m_focus->length() * LABEL_MIN_ANGLE_FACTOR);

        //**** Levels should be on a scale starting with 0
        //**** range is a useless parameter
        //**** keep a topblock var which is the lowestLevel OR startLevel for indentation purposes
        for (unsigned int i = startLevel; i <= m_map.m_visibleDepth; ++i) {
            for (const Segment *segment : m_map.m_signature[i]) {
                if (segment->start() >= start && segment->end() <= end) {
                    if (segment->length() > minAngle) {
                        list.append(new Label(segment, i));
                    }
                }
            }
        }
    } else {
        for (Segment *segment : m_map.m_signature[0]) {
            if (segment->length() > 288) {
                list.append(new Label(segment, 0));
            }
        }
    }

    std::sort(list.begin(), list.end(), [](Label *item1, Label *item2) {
        // you add 1440 to work round the fact that later you want the circle split vertically
        // and as it is you start at 3 o' clock. It's to do with rightPrevY, stops annoying bug

        int angle1 = (item1)->angle + 1440;
        int angle2 = (item2)->angle + 1440;

        // Also sort by level
        if (angle1 == angle2) {
            return (item1->level > item2->level);
        }

        if (angle1 > 5760) {
            angle1 -= 5760;
        }
        if (angle2 > 5760) {
            angle2 -= 5760;
        }

        return (angle1 < angle2);
    });

    // 2. Check to see if any adjacent labels are too close together
    //    if so, remove it (the least significant labels, since we sort by level too).

    int pos = 0;
    while (pos < list.size() - 1) {
        if (list[pos]->tooClose(list[pos + 1]->angle)) {
            delete list.takeAt(pos + 1);
        } else {
            ++pos;
        }
    }

    // used in next two steps
    bool varySizes = 0;
    //**** should perhaps use doubles
    int *sizes = new int[m_map.m_visibleDepth + 1]; //**** make sizes an array of floats I think instead (or doubles)

    // If the minimum is larger than the default it fucks up further down
    if (paint.font().pointSize() < 0 || paint.font().pointSize() < Config::minFontPitch) {
        QFont font = paint.font();
        font.setPointSize(Config::minFontPitch);
        paint.setFont(font);
    }

    QVector<Label *>::iterator it;

    do {
        // 3. Calculate font sizes

        {
            // determine current range of levels to draw for
            uint range = 0;

            for (const auto &label : std::as_const(list)) {
                range = qMax(range, label->level);

                //**** better way would just be to assign if nothing is range
            }

            range -= startLevel; // range 0 means 1 level of labels

            varySizes = Config::varyLabelFontSizes && (range != 0);

            if (varySizes) {
                // create an array of font sizes for various levels
                // will exceed normal font pitch automatically if necessary, but not minPitch
                //**** this needs to be checked lots

                //**** what if this is negative (min size gtr than default size)
                uint step = (paint.font().pointSize() - Config::minFontPitch) / range;
                if (step == 0) {
                    step = 1;
                }

                for (uint x = range + startLevel, y = Config::minFontPitch; x >= startLevel; y += step, --x) {
                    sizes[x] = y;
                }
            }
        }

        // 4. determine label co-ordinates

        const int preSpacer = int(m_map.m_ringBreadth * 0.5) + m_map.m_innerRadius;
        const int fullStrutLength = (m_map.width() - m_map.MAP_2MARGIN) / 2 + LABEL_MAP_SPACER; // full length of a strut from map center

        int prevLeftY = 0;
        int prevRightY = height();

        QFont font;

        for (it = list.begin(); it != list.end(); ++it) {
            Label *label = *it;
            //** bear in mind that text is drawn with QPoint param as BOTTOM left corner of text box
            QString string = label->segment->file()->displayName();
            if (varySizes) {
                font.setPointSize(sizes[label->level]);
            }
            QFontMetrics fontMetrics(font);
            const int minTextWidth = fontMetrics.boundingRect(QStringLiteral("M...")).width() + LABEL_TEXT_HMARGIN; // Fully elided string

            const int fontHeight = fontMetrics.height() + LABEL_TEXT_VMARGIN; // used to ensure label texts don't overlap
            const int lineSpacing = fontHeight / 4;

            const bool rightSide = (label->angle < 1440 || label->angle > 4320);

            double sinra;
            double cosra;
            const double ra = M_PI / 2880 * label->angle; // convert to radians
            sincos(ra, &sinra, &cosra);

            const int spacer = preSpacer + m_map.m_ringBreadth * label->level;

            const int centerX = m_map.width() / 2 + m_offset.x(); // centre relative to canvas
            const int centerY = m_map.height() / 2 + m_offset.y();
            int targetX = centerX + cosra * spacer;
            int targetY = centerY - sinra * spacer;
            int startX = targetX + cosra * (fullStrutLength - spacer + m_map.m_ringBreadth / 2);
            int startY = targetY - sinra * (fullStrutLength - spacer);

            if (rightSide) { // righthand side, going upwards
                if (startY > prevRightY /*- fmh*/) { // then it is too low, needs to be drawn higher
                    startY = prevRightY /*- fmh*/;
                }
            } else { // lefthand side, going downwards
                if (startY < prevLeftY /* + fmh*/) { // then we're too high, need to be drawn lower
                    startY = prevLeftY /*+ fmh*/;
                }
            }

            int middleX = targetX - (tan(ra) > 0 ? (startY - targetY) / tan(ra) : 0);
            int textY = startY + lineSpacing;

            int textX = 0;
            const int textWidth = fontMetrics.boundingRect(string).width() + LABEL_TEXT_HMARGIN;
            if (rightSide) {
                if (startX + minTextWidth > width() || textY < fontHeight || middleX < targetX) {
                    // skip this strut
                    //**** don't duplicate this code
                    it = list.erase(it); // will delete the label and set it to list.current() which _should_ be the next ptr
                    delete label;
                    break;
                }

                prevRightY = textY - fontHeight - lineSpacing; // must be after above's "continue"

                if (m_offset.x() + m_map.width() + textWidth < width()) {
                    startX = m_offset.x() + m_map.width();
                } else {
                    startX = qMax<int>(width() - textWidth, startX);
                    string = fontMetrics.elidedText(string, Qt::ElideMiddle, width() - startX);
                }

                textX = startX + LABEL_TEXT_HMARGIN;
            } else { // left side
                if (startX - minTextWidth < 0 || textY > height() || middleX > targetX) {
                    // skip this strut
                    it = list.erase(it); // will delete the label and set it to list.current() which _should_ be the next ptr
                    delete label;
                    break;
                }

                prevLeftY = textY + fontHeight - lineSpacing;

                if (m_offset.x() - textWidth > 0) {
                    startX = m_offset.x();
                    textX = startX - textWidth - LABEL_TEXT_HMARGIN;
                } else {
                    textX = 0;
                    string = fontMetrics.elidedText(string, Qt::ElideMiddle, startX);
                    startX = fontMetrics.boundingRect(string).width() + LABEL_TEXT_HMARGIN;
                }
            }

            label->targetX = targetX;
            label->targetY = targetY;
            label->middleX = middleX;
            label->startY = startY;
            label->startX = startX;
            label->textX = textX;
            label->textY = textY;
            label->qs = string;
        }

        // if an element is deleted at this stage, we need to do this whole
        // iteration again, thus the following loop
        //**** in rare case that deleted label was last label in top level
        //      and last in labelList too, this will not work as expected (not critical)

    } while (it != list.end());

    // 5. Render labels

    QFont font;
    for (const auto &label : std::as_const(list)) {
        if (varySizes) {
            //**** how much overhead in making new QFont each time?
            //     (implicate sharing remember)
            font.setPointSize(sizes[label->level]);
            paint.setFont(font);
        }

        paint.drawLine(label->targetX, label->targetY, label->middleX, label->startY);
        paint.drawLine(label->middleX, label->startY, label->startX, label->startY);

        paint.drawText(label->textX, label->textY, label->qs);
    }

    qDeleteAll(list);
    delete[] sizes;
}

void RadialMap::Item::hoverEnterEvent(QHoverEvent * /*event*/)
{
    if (!m_focus) {
        return;
    }

    setCursor(Qt::PointingHandCursor);
    Q_EMIT mouseHover(m_focus->file()->displayPath());
    update();
}

void RadialMap::Item::hoverLeaveEvent(QHoverEvent * /*event*/)
{
}

void RadialMap::Item::dropEvent(QDropEvent *e)
{
    QList<QUrl> uriList = KUrlMimeData::urlsFromMimeData(e->mimeData());
    if (!uriList.isEmpty()) {
        Q_EMIT giveMeTreeFor(uriList.first());
    }
}

void RadialMap::Item::dragEnterEvent(QDragEnterEvent *e)
{
    QList<QUrl> uriList = KUrlMimeData::urlsFromMimeData(e->mimeData());
    if (!uriList.isEmpty()) {
        e->acceptProposedAction();
    }
}

bool RadialMap::Item::event(QEvent *e)
{
    if (e->type() == QEvent::ApplicationPaletteChange || e->type() == QEvent::PaletteChange) {
        m_map.paint();
    }
    return QQuickPaintedItem::event(e);
}

void RadialMap::Item::hoverMoveEvent(QHoverEvent *e)
{
    // set m_focus to what we hover over, update UI if it's a new segment

    Segment const *const oldFocus = m_focus;
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

    const QRectF screenRect = window()->screen()->availableGeometry();

    QPoint tooltipPosition = window()->mapToGlobal(e->pos()) + QPoint(20, 20);
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

    QString string = i18nc("Tooltip of file/folder, %1 is path, %2 is size", "%1\n%2", m_focus->file()->displayPath(), m_focus->file()->humanReadableSize());

    if (m_focus->file()->isFolder()) {
        int files = std::dynamic_pointer_cast<Folder>(m_focus->file())->children();
        const uint percent = uint((100 * files) / (double)m_tree->children());

        string += QLatin1Char('\n');
        if (percent > 0) {
            string += i18ncp("Tooltip of folder, %1 is number of files", "%1 File (%2%)", "%1 Files (%2%)", files, percent);
        } else {
            string += i18ncp("Tooltip of folder, %1 is number of files", "%1 File", "%1 Files", files);
        }
    }

    const QUrl url = Item::url(m_focus->file());
    if (m_focus == m_rootSegment.get() && url != KIO::upUrl(url)) {
        string += i18n("\nClick to go up to parent directory");
    }
    // Calculate a semi-sane size for the tooltip
    QFontMetrics fontMetrics(m_tooltip.fontMetrics());
    int tooltipWidth = 0;
    int tooltipHeight = 0;
    const auto parts = string.split(QLatin1Char('\n'));
    for (const QString &part : parts) {
        tooltipHeight += fontMetrics.height();
        tooltipWidth = qMax(tooltipWidth, fontMetrics.horizontalAdvance(part));
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

const RadialMap::Segment *RadialMap::Item::segmentAt(QPointF e) const
{
    // determine which segment QPointF e is above

    e -= m_offset;

    if (m_map.m_signature.isEmpty()) {
        return nullptr;
    }

    if (e.x() <= m_map.width() && e.y() <= m_map.height()) {
        // transform to cartesian coords
        e.rx() -= m_map.width() / 2; // should be an int
        e.ry() = m_map.height() / 2 - e.y();

        double length = hypot(e.x(), e.y());

        if (length >= m_map.m_innerRadius) // not hovering over inner circle
        {
            uint depth = ((int)length - m_map.m_innerRadius) / m_map.m_ringBreadth;

            if (depth <= m_map.m_visibleDepth) //**** do earlier since you can //** check not outside of range
            {
                // vector calculation, reduces to simple trigonometry
                // cos angle = (aibi + ajbj) / albl
                // ai = x, bi=1, aj=y, bj=0
                // cos angle = x / (length)

                uint a = (uint)(acos((double)e.x() / length) * 916.736); // 916.7324722 = #radians in circle * 16

                // acos only understands 0-180 degrees
                if (e.y() < 0) {
                    a = 5760 - a;
                }

                for (Segment *segment : m_map.m_signature[depth]) {
                    if (segment->intersects(a)) {
                        return segment;
                    }
                }
            }
        } else {
            return m_rootSegment.get(); // hovering over inner circle
        }
    }

    return nullptr;
}

void RadialMap::Item::saveSVG()
{
    const QString path = QFileDialog::getSaveFileName(nullptr,
                                                      i18nc("@title:window", "Save as SVG"),
                                                      QString(),
                                                      i18nc("filedialog type filter", "SVG Files (*.svg);;All Files(*)"));
    if (!path.isEmpty()) {
        m_map.saveSvg(path);
    }
}

const RadialMap::Segment *RadialMap::Item::focusSegment() const
{
    return m_focus; /// 0 == nothing in focus
}

const RadialMap::Segment *RadialMap::Item::rootSegment() const
{
    return m_rootSegment.get(); /// never == 0
}

uint RadialMap::Item::treeChildren()
{
    return m_tree->children();
}
