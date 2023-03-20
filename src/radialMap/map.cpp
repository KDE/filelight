/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "filelight_debug.h"

#include <array>
#include <utility>

#ifdef Q_OS_WINDOWS
#include <winrt/Windows.UI.ViewManagement.h>
#pragma comment(lib, "windowsapp")
#endif

#include <QApplication> //make()
#include <QFont> //ctor
#include <QFontMetrics> //ctor
#include <QImage> //make() & paint()
#include <QPainter>

#include <KColorScheme>
#include <KLocalizedString>

#include "map.h"
#include "radialMap.h" // defines
#include "sincos.h"

RadialMap::Map::Map()
    : m_visibleDepth(DEFAULT_RING_DEPTH)
    , m_ringBreadth(MIN_RING_BREADTH)
    , m_innerRadius(0)
{
    // FIXME this is all broken. No longer is a maximum depth!
    const int fmh = QFontMetrics(QFont()).height();
    const int fmhD4 = fmh / 4;
    MAP_2MARGIN = 2 * (fmh - (fmhD4 - LABEL_MAP_SPACER)); // margin is dependent on fitting in labels at top and bottom
    // Initialize breadth
    resize(QRectF());

    connect(qGuiApp, &QGuiApplication::paletteChanged, this, [this] {
        colorise();
    });
}

// Helps to represent a group of files like a single segment on the map
class FilesGroup : public File
{
public:
    FilesGroup(int fileCount, FileSize totalSize, Folder *parent)
        : File("", totalSize, parent)
    {
        const QString fakeName = i18np("\n%1 file, with an average size of %2",
                                       "\n%1 files, with an average size of %2",
                                       fileCount,
                                       KFormat().formatByteSize(totalSize / fileCount));
        m_name = fakeName.toUtf8().constData();
    };
};

namespace
{
enum class Delete { Now = true, Later = false };
void deleteAllSegments(QVector<QList<RadialMap::Segment *>> &signature, Delete del = Delete::Later)
{
    for (auto &segments : signature) {
        for (const auto &segment : segments) {
            if (segment->file()) {
                Q_ASSERT(segment->file()->segment() == segment->uuid());
                segment->file()->setSegment({});
            }
            del == Delete::Now ? delete segment : segment->deleteLater();
        }
    }
    signature.clear();
}
} // namespace

RadialMap::Map::~Map()
{
    deleteAllSegments(m_signature, Delete::Now);
}

void RadialMap::Map::invalidate()
{
    deleteAllSegments(m_signature);
    Q_EMIT signatureChanged();

    m_visibleDepth = Config::instance()->defaultRingDepth;
}

void RadialMap::Map::make(const std::shared_ptr<Folder> &tree, bool refresh)
{
    // slow operation so set the wait cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // build a signature of visible components
    {
        //**** REMOVE NEED FOR the +1 with MAX_RING_DEPTH uses
        //**** add some angle bounds checking (possibly in Segment ctor? can I delete in a ctor?)
        //**** this is a mess

        deleteAllSegments(m_signature);
        m_signature.resize(m_visibleDepth + 1);
        Q_EMIT signatureChanged();

        m_root = tree;
        if (m_rootSegment && m_rootSegment->file()) {
            m_rootSegment->file()->setSegment({});
        }
        m_rootSegment = std::make_unique<Segment>(tree, 0, MAX_DEGREE);

        if (!refresh) {
            m_minSize = (tree->size() * 3) / (PI * height() - MAP_2MARGIN);
            findVisibleDepth(tree);
        }

        setRingBreadth();

        // Calculate ring size limits
        m_limits.resize(m_visibleDepth + 1);
        const double size = m_root->size();
        const double pi2B = M_PI * 4 * m_ringBreadth;
        for (uint depth = 0; depth <= m_visibleDepth; ++depth) {
            m_limits[depth] = uint(size / double(pi2B * (depth + 1))); // min is angle that gives 3px outer diameter for that depth
        }

        build(tree);
    }

    // colour the segments
    colorise();

    QApplication::restoreOverrideCursor();
}

void RadialMap::Map::setRingBreadth()
{
    // FIXME called too many times on creation

    m_ringBreadth = (height() - MAP_2MARGIN) / (2 * m_visibleDepth + 4);
    m_ringBreadth = qBound(MIN_RING_BREADTH, m_ringBreadth, MAX_RING_BREADTH);
}

void RadialMap::Map::findVisibleDepth(const std::shared_ptr<Folder> &dir, uint currentDepth)
{
    //**** because I don't use the same minimumSize criteria as in the visual function
    //     this can lead to incorrect visual representation
    //**** BUT, you can't set those limits until you know m_depth!

    //**** also this function doesn't check to see if anything is actually visible
    //     it just assumes that when it reaches a new level everything in it is visible
    //     automatically. This isn't right especially as there might be no files in the
    //     dir provided to this function!

    static uint stopDepth = 0;

    if (dir == m_root) {
        stopDepth = m_visibleDepth;
        m_visibleDepth = 0;
    }

    if (m_visibleDepth < currentDepth) {
        qCDebug(FILELIGHT_LOG) << "changing visual depth" << m_visibleDepth << currentDepth;
        m_visibleDepth = currentDepth;
    }
    if (m_visibleDepth >= stopDepth) {
        return;
    }

    for (const auto &file : dir->files) {
        if (file->isFolder() && file->size() > m_minSize) {
            findVisibleDepth(std::dynamic_pointer_cast<Folder>(file), currentDepth + 1); // if no files greater than min size the depth is still recorded
        }
    }
}

//**** segments currently overlap at edges (i.e. end of first is start of next)
bool RadialMap::Map::build(const std::shared_ptr<Folder> &dir, const uint depth, uint a_start, const uint a_end)
{
    // first iteration: dir == m_root

    if (dir->children() == 0) { // we do fileCount rather than size to avoid chance of divide by zero later
        return false;
    }

    FileSize hiddenSize = 0;
    uint hiddenFileCount = 0;

    for (const auto &file : dir->files) {
        if (file->size() < m_limits[depth] * 6) { // limit is half a degree? we want at least 3 degrees
            hiddenSize += file->size();
            if (file->isFolder()) { //**** considered virtual, but dir wouldn't count itself!
                hiddenFileCount += std::dynamic_pointer_cast<Folder>(file)->children(); // need to add one to count the dir as well
            }
            ++hiddenFileCount;
            continue;
        }

        auto a_len = (unsigned int)(MAX_DEGREE * ((double)file->size() / (double)m_root->size()));

        auto *s = new Segment(file, a_start, a_len);
        m_signature[depth].append(s);

        if (file->isFolder()) {
            if (depth != m_visibleDepth) {
                // recurse
                s->m_hasHiddenChildren = build(std::dynamic_pointer_cast<Folder>(file), depth + 1, a_start, a_start + a_len);
            } else {
                s->m_hasHiddenChildren = true;
            }
        }

        a_start += a_len; //**** should we add 1?
    }
    if (depth == 0) {
        Q_EMIT signatureChanged();
    }

    if (hiddenFileCount == dir->children() && !Config::instance()->showSmallFiles) {
        return true;
    }

    if ((depth == 0 || Config::instance()->showSmallFiles) && hiddenSize >= m_limits[depth] && hiddenFileCount > 0) {
        m_signature[depth].append(new Segment(std::make_shared<FilesGroup>(hiddenFileCount, hiddenSize, dir.get()), a_start, a_end - a_start, true));
        Q_EMIT signatureChanged();
    }

    return false;
}

bool RadialMap::Map::resize(const QRectF &newRect)
{
    // there's a MAP_2MARGIN border

    if (newRect.width() < width() && newRect.height() < height() && !newRect.contains(m_rect)) {
        return false;
    }

    qreal size = qMin(newRect.width(), newRect.height()) - MAP_2MARGIN;

    // this also causes uneven sizes to always resize when resizing but map is small in that dimension
    // size -= size % 2; //even sizes mean less staggered non-antialiased resizing

    const uint minSize = MIN_RING_BREADTH * 2 * (m_visibleDepth + 2);

    if (size < minSize) {
        size = minSize;
    }

    // this QRectF is used by paint()
    m_rect.setRect(0, 0, size, size);
    Q_EMIT rectChanged();

    // resize the pixmap
    size += MAP_2MARGIN;

    if (!m_signature.isEmpty()) {
        setRingBreadth();
    }

    return true;
}

void RadialMap::Map::colorise()
{
    if (m_signature.isEmpty()) {
        qCDebug(FILELIGHT_LOG) << "no signature yet";
        return;
    }

    QColor cp;
    QColor cb;
    double darkness = 1;
    double contrast = (double)Config::instance()->contrast / (double)100;
    int h = 0;
    int s1 = 0;
    int s2 = 0;
    int v1 = 0;
    int v2 = 0;

    QPalette palette = qGuiApp->palette();
#ifdef Q_OS_WINDOWS
    winrt::Windows::UI::ViewManagement::UISettings settings;
    winrt::Windows::UI::Color color = settings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Accent);
    const QColor kdeColour[2] = {palette.window().color(), QColor(color.R, color.G, color.B, color.A)};
#else
    const QColor kdeColour[2] = {palette.windowText().color(), palette.window().color()};
#endif

    double deltaRed = (double)(kdeColour[0].red() - kdeColour[1].red()) / 2880; // 2880 for semicircle
    double deltaGreen = (double)(kdeColour[0].green() - kdeColour[1].green()) / 2880;
    double deltaBlue = (double)(kdeColour[0].blue() - kdeColour[1].blue()) / 2880;

    for (uint i = 0; i <= m_visibleDepth; ++i, darkness += 0.04) {
        for (const auto &segment : std::as_const(m_signature[i])) {
            switch (Config::instance()->scheme) {
            case Filelight::KDE: {
                // gradient will work by figuring out rgb delta values for 360 degrees
                // then each component is angle*delta

                int a = segment->start();

                if (a > 2880) {
                    a = 2880 - (a - 2880);
                }

                h = (int)(deltaRed * a) + kdeColour[1].red();
                s1 = (int)(deltaGreen * a) + kdeColour[1].green();
                v1 = (int)(deltaBlue * a) + kdeColour[1].blue();

                cb.setRgb(h, s1, v1);
                cb.getHsv(&h, &s1, &v1);

                break;
            }

            case Filelight::HighContrast:
                cp.setHsv(0, 0, 0); // values of h, s and v are irrelevant
                cb.setHsv(180, 0, int(255.0 * contrast));
                segment->setPalette(cp, cb);
                continue;

            default:
                h = int(segment->start() / DEGREE_FACTOR);
                s1 = 160;
                v1 = (int)(255.0 / darkness); //****doing this more often than once seems daft!
            }

            v2 = v1 - int(contrast * v1);
            s2 = s1 + int(contrast * (255 - s1));

            if (s1 < 80) {
                s1 = 80; // can fall too low and makes contrast between the files hard to discern
            }

            if (segment->isFake()) { // multi-file
                cb.setHsv(h, s2, (v2 < 90) ? 90 : v2); // too dark if < 100
                cp.setHsv(h, 17, v1);
            } else if (!segment->file()->isFolder()) { // file
                cb.setHsv(h, 17, v1);
                cp.setHsv(h, 17, v2);
            } else { // folder
                cb.setHsv(h, s1, v1); // v was 225
                cp.setHsv(h, s2, v2); // v was 225 - delta
            }

            segment->setPalette(cp, cb);

            // TODO:
            //**** may be better to store KDE colours as H and S and vary V as others
            //**** perhaps make saturation difference for s2 dependent on contrast too
            //**** fake segments don't work with highContrast
            //**** may work better with cp = cb rather than Qt::white
            //**** you have to ensure the grey of files is sufficient, currently it works only with rainbow (perhaps use contrast there too)
            //**** change v1,v2 to vp, vb etc.
            //**** using percentages is not strictly correct as the eye doesn't work like that
            //**** darkness factor is not done for kde_colour scheme, and also value for files is incorrect really for files in this scheme as it is not set
            // like rainbow one is
        }
    }
}

QList<QVariant> RadialMap::Map::signature()
{
    QList<QVariant> ret;

    for (auto &list : m_signature) {
        QList<QObject *> r;

        for (auto &element : list) {
            r << element;
        }
        ret << QVariant::fromValue(r);
        // break;
    }
    return ret;
}

void RadialMap::Map::zoomIn() // slot
{
    if (m_visibleDepth > MIN_RING_DEPTH) {
        --m_visibleDepth;
        make(m_root);
        Config::instance()->defaultRingDepth = m_visibleDepth;
    }
}

void RadialMap::Map::zoomOut() // slot
{
    ++m_visibleDepth;
    make(m_root);
    if (m_visibleDepth > Config::instance()->defaultRingDepth) {
        Config::instance()->defaultRingDepth = m_visibleDepth;
    }
}

void RadialMap::Map::refresh(const Filelight::Dirty filth)
{
    // TODO consider a more direct connection

    if (!isNull()) {
        switch (filth) {
        case Filelight::Dirty::Layout:
            make(m_root, true); // true means refresh only
            break;
        case Filelight::Dirty::Colors:
            colorise();
            break;
        }
    }
}

void RadialMap::Map::createFromCache(const std::shared_ptr<Folder> &tree)
{
    qCDebug(FILELIGHT_LOG) << "Creating cached tree";
    // no scan was necessary, use cached tree, however we MUST still emit invalidate
    invalidate();
    make(tree);
}

void RadialMap::Map::createFromCacheObject(RadialMap::Segment *segment)
{
    createFromCache(std::dynamic_pointer_cast<Folder>(segment->file()));
}

QUrl RadialMap::Map::rootUrl() const
{
    return m_root ? m_root->url() : QUrl();
}

QObject *RadialMap::Map::rootSegment() const
{
    return m_rootSegment.get();
}
