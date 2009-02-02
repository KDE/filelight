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

#include <KCursor>         //make()
#include <KDebug>
#include <KGlobalSettings> //kdeColours
#include <qimageblitz/qimageblitz.h>    //desaturate()
#include <QApplication>    //make()
#include <QImage>          //make() & paint()
#include <QFont>           //ctor
#include <QFontMetrics>    //ctor
#include <QPainter>
#include <Q3PointArray>

#include "builder.h"
#include "part/Config.h"
#include "part/fileTree.h"
#define SINCOS_H_IMPLEMENTATION (1)
#include "sincos.h"
#include "widget.h"

RadialMap::Map::Map()
        : m_signature(0)
        , m_visibleDepth(DEFAULT_RING_DEPTH)
        , m_pixmap()
        , m_ringBreadth(MIN_RING_BREADTH)
        , m_innerRadius(0)
{
    //FIXME this is all broken. No longer is a maximum depth!
    const int fmh   = QFontMetrics(QFont()).height();
    const int fmhD4 = fmh / 4;
    MAP_2MARGIN = 2 * (fmh - (fmhD4 - LABEL_MAP_SPACER)); //margin is dependent on fitting in labels at top and bottom
}

RadialMap::Map::~Map()
{
    delete [] m_signature;
}

void RadialMap::Map::invalidate(const bool desaturateTheImage)
{
    delete [] m_signature;
    m_signature = 0;

    if (desaturateTheImage)
    {
        QImage img = m_pixmap.toImage();

        Blitz::desaturate(img, 0.7);
        Blitz::grayscale(img, true);

        m_pixmap.fromImage(img);
    }

    m_visibleDepth = Config::defaultRingDepth;
}

void RadialMap::Map::make(const Directory *tree, bool refresh)
{
    //**** determineText seems pointless optimisation
    //   but is it good to keep the text consistent?
    //   even if it makes it a lie?

    //slow operation so set the wait cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);

    {
        //build a signature of visible components
        delete [] m_signature;
        Builder builder(this, tree, refresh);
    }

    //colour the segments
    colorise();

    //determine centerText
    if (!refresh)
    {
        int i;
        for (i = 2; i > 0; --i)
            if (tree->size() > File::DENOMINATOR[i])
                break;

        m_centerText = tree->humanReadableSize((File::UnitPrefix)i);
    }

    //paint the pixmap
    aaPaint();

    QApplication::restoreOverrideCursor();
}

void RadialMap::Map::setRingBreadth()
{
    //FIXME called too many times on creation

    m_ringBreadth = (height() - MAP_2MARGIN) / (2 * m_visibleDepth + 4);

    if (m_ringBreadth < MIN_RING_BREADTH)
        m_ringBreadth = MIN_RING_BREADTH;

    else if (m_ringBreadth > MAX_RING_BREADTH)
        m_ringBreadth = MAX_RING_BREADTH;
}

bool RadialMap::Map::resize(const QRect &rect)
{
    //there's a MAP_2MARGIN border

#define mw width()
#define mh height()
#define cw rect.width()
#define ch rect.height()

    if (cw < mw || ch < mh || (cw > mw && ch > mh))
    {
        uint size = ((cw < ch) ? cw : ch) - MAP_2MARGIN;

        //this also causes uneven sizes to always resize when resizing but map is small in that dimension
        //size -= size % 2; //even sizes mean less staggered non-antialiased resizing

        {
            const uint minSize = MIN_RING_BREADTH * 2 * (m_visibleDepth + 2);

            if (size < minSize)
                size = minSize;

            //this QRect is used by paint()
            m_rect.setRect(0,0,size,size);
        }

        //resize the pixmap
        size += MAP_2MARGIN;
        m_pixmap = QPixmap(size, size);

        // for summary widget this is a good optimisation as it happens
        if (m_pixmap.isNull())
            return false;

        if (m_signature != 0)
        {
            setRingBreadth();
            paint();
        }
        else m_pixmap.fill(); //FIXME I don't like having to do this..

        return true;
    }

#undef mw
#undef mh
#undef cw
#undef ch

    return false;
}

void RadialMap::Map::colorise()
{
    QColor cp, cb;
    double darkness = 1;
    double contrast = (double)Config::contrast / (double)100;
    int h, s1, s2, v1, v2;

    QColor kdeColour[2] = { KGlobalSettings::inactiveTitleColor(), KGlobalSettings::activeTitleColor() };

    double deltaRed   = (double)(kdeColour[0].red()   - kdeColour[1].red())   / 2880; //2880 for semicircle
    double deltaGreen = (double)(kdeColour[0].green() - kdeColour[1].green()) / 2880;
    double deltaBlue  = (double)(kdeColour[0].blue()  - kdeColour[1].blue())  / 2880;

    for (uint i = 0; i <= m_visibleDepth; ++i, darkness += 0.04)
    {
        for (Iterator<Segment> it = m_signature[i].iterator(); it != m_signature[i].end(); ++it)
        {
            switch (Config::scheme)
            {
            case 2000: //HACK for summary view

                if ((*it)->file()->name() == "Used") {
                    cb = QApplication::palette().highlight().color();
                    cb.getHsv(&h, &s1, &v1);

                    if (s1 > 80)
                        s1 = 80;

                    v2 = v1 - int(contrast * v1);
                    s2 = s1 + int(contrast * (255 - s1));

                    cb.setHsv(h, s1, v1);
                    cp.setHsv(h, s2, v2);
                }
                else {
                    cp = Qt::gray;
                    cb = Qt::white;
                }

                (*it)->setPalette(cp, cb);

                continue;

            case Filelight::KDE:
            {
                //gradient will work by figuring out rgb delta values for 360 degrees
                //then each component is angle*delta

                int a = (*it)->start();

                if (a > 2880) a = 2880 - (a - 2880);

                h  = (int)(deltaRed   * a) + kdeColour[1].red();
                s1 = (int)(deltaGreen * a) + kdeColour[1].green();
                v1 = (int)(deltaBlue  * a) + kdeColour[1].blue();

                cb.setRgb(h, s1, v1);
                cb.getHsv(&h, &s1, &v1);

                break;
            }

            case Filelight::HighContrast:

                cp.setHsv(0, 0, 0); //values of h, s and v are irrelevant
                cb.setHsv(180, 0, int(255.0 * contrast));
                (*it)->setPalette(cp, cb);
                continue;

            default:
                h  = int((*it)->start() / 16);
                s1 = 160;
                v1 = (int)(255.0 / darkness); //****doing this more often than once seems daft!
            }

            v2 = v1 - int(contrast * v1);
            s2 = s1 + int(contrast * (255 - s1));

            if (s1 < 80) s1 = 80; //can fall too low and makes contrast between the files hard to discern

            if ((*it)->isFake()) //multi-file
            {
                cb.setHsv(h, s2, (v2 < 90) ? 90 : v2); //too dark if < 100
                cp.setHsv(h, 17, v1);
            }
            else if (!(*it)->file()->isDirectory()) //file
            {
                cb.setHsv(h, 17, v1);
                cp.setHsv(h, 17, v2);
            }
            else //directory
            {
                cb.setHsv(h, s1, v1); //v was 225
                cp.setHsv(h, s2, v2); //v was 225 - delta
            }

            (*it)->setPalette(cp, cb);

            //TODO:
            //**** may be better to store KDE colours as H and S and vary V as others
            //**** perhaps make saturation difference for s2 dependent on contrast too
            //**** fake segments don't work with highContrast
            //**** may work better with cp = cb rather than Qt::white
            //**** you have to ensure the grey of files is sufficient, currently it works only with rainbow (perhaps use contrast there too)
            //**** change v1,v2 to vp, vb etc.
            //**** using percentages is not strictly correct as the eye doesn't work like that
            //**** darkness factor is not done for kde_colour scheme, and also value for files is incorrect really for files in this scheme as it is not set like rainbow one is
        }
    }
}

void RadialMap::Map::aaPaint()
{
    //paint() is called during continuous processes
    //aaPaint() is not and is slower so set overidecursor (make sets it too)
    QApplication::setOverrideCursor(Qt::WaitCursor);
    paint(Config::antiAliasFactor);
    QApplication::restoreOverrideCursor();
}

void RadialMap::Map::paint(unsigned int scaleFactor)
{
    if (scaleFactor == 0) //just in case
        scaleFactor = 1;

    QPainter paint;
    QRect rect = m_rect;
    int step = m_ringBreadth;
    int excess = -1;

    //scale the pixmap, or do intelligent distribution of excess to prevent nasty resizing
    if (scaleFactor > 1)
    {
        int x1, y1, x2, y2;
        rect.getCoords(&x1, &y1, &x2, &y2);
        x1 *= scaleFactor;
        y1 *= scaleFactor;
        x2 *= scaleFactor;
        y2 *= scaleFactor;
        rect.setCoords(x1, y1, x2, y2);

        step *= scaleFactor;
        m_pixmap = QPixmap(m_pixmap.size() * (int)scaleFactor);
    }
    else if (m_ringBreadth != MAX_RING_BREADTH && m_ringBreadth != MIN_RING_BREADTH) {
        excess = rect.width() % m_ringBreadth;
        ++step;
    }

    //**** best option you can think of is to make the circles slightly less perfect,
    //  ** i.e. slightly eliptic when resizing inbetween

    if (m_pixmap.isNull()) {
        kWarning() << "Refusing to draw on null pixmap.";
        return;
    }

    paint.begin(&m_pixmap);

    m_pixmap.fill(); //erase background

    for (int x = m_visibleDepth; x >= 0; --x)
    {
        int width = rect.width() / 2;
        //clever geometric trick to find largest angle that will give biggest arrow head
        int a_max = int(acos((double)width / double((width + 5) * scaleFactor)) * (180*16 / M_PI));

        for (ConstIterator<Segment> it = m_signature[x].constIterator(); it != m_signature[x].end(); ++it)
        {
            //draw the pie segments, most of this code is concerned with drawing the little
            //arrows on the ends of segments when they have hidden files

            paint.setPen((*it)->pen());

            if ((*it)->hasHiddenChildren())
            {
                //draw arrow head to indicate undisplayed files/directories
                Q3PointArray pts(3);
                QPoint pos, cpos = rect.center();
                int a[3] = { (*it)->start(), (*it)->length(), 0 };

                a[2] = a[0] + (a[1] / 2); //assign to halfway between
                if (a[1] > a_max)
                {
                    a[1] = a_max;
                    a[0] = a[2] - a_max / 2;
                }

                a[1] += a[0];

                for (int i = 0, radius = width; i < 3; ++i)
                {
                    double ra = M_PI/(180*16) * a[i], sinra, cosra;

                    if (i == 2)
                        radius += 5 * scaleFactor;
                    sincos(ra, &sinra, &cosra);
                    pos.rx() = cpos.x() + static_cast<int>(cosra * radius);
                    pos.ry() = cpos.y() - static_cast<int>(sinra * radius);
                    pts.setPoint(i, pos);
                }

                paint.setBrush((*it)->pen());
                paint.drawPolygon(pts);
            }

            paint.setBrush((*it)->brush());
            paint.drawPie(rect, (*it)->start(), (*it)->length());

            if ((*it)->hasHiddenChildren())
            {
                //**** code is bloated!
                paint.save();
                QPen pen = paint.pen();
                int width = 2 * scaleFactor;
                pen.setWidth(width);
                paint.setPen(pen);
                QRect rect2 = rect;
                width /= 2;
                rect2.adjust(width, width, -width, -width);
                paint.drawArc(rect2, (*it)->start(), (*it)->length());
                paint.restore();
            }
        }

        if (excess >= 0) { //excess allows us to resize more smoothly (still crud tho)
            if (excess < 2) //only decrease rect by more if even number of excesses left
                --step;
            excess -= 2;
        }

        rect.adjust(step, step, -step, -step);
    }

    //  if(excess > 0) rect.addCoords(excess, excess, 0, 0); //ugly

    paint.setPen(Qt::gray);
    paint.setBrush(Qt::white);
    paint.drawEllipse(rect);
    if (scaleFactor > 1)
    {
        //have to end in order to smoothscale()
        paint.end();

        int x1, y1, x2, y2;
        rect.getCoords(&x1, &y1, &x2, &y2);
        x1 /= scaleFactor;
        y1 /= scaleFactor;
        x2 /= scaleFactor;
        y2 /= scaleFactor;
        rect.setCoords(x1, y1, x2, y2);

        QImage img = m_pixmap.toImage();
        img = img.scaled(m_pixmap.size() / (int)scaleFactor, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_pixmap = QPixmap::fromImage(img);

        paint.begin(&m_pixmap);
        paint.setPen(Qt::gray);
        paint.setBrush(Qt::white);
    }

    paint.drawText(rect, Qt::AlignCenter, m_centerText);

    m_innerRadius = rect.width() / 2; //rect.width should be multiple of 2

    paint.end();
}
