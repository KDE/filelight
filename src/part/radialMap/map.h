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

#ifndef MAP_H
#define MAP_H

#include <QPixmap>
#include <QRect>
#include <QString>

namespace RadialMap {
class Segment;

class Map
{
public:
    Map();
    ~Map();

    void make(const Directory *, bool = false);
    bool resize(const QRect&);

    bool isNull() const {
        return (m_signature == 0);
    }
    void invalidate(const bool);

    int height() const {
        return m_rect.height();
    }
    int width() const {
        return m_rect.width();
    }

    QPixmap getPixmap() {
        return m_pixmap;
    }

    friend class Builder;
    friend class Widget;

private:
    void paint(uint = 1);
    void aaPaint();
    void colorise();
    void setRingBreadth();

    Chain<Segment> *m_signature;

    QRect   m_rect;
    uint    m_visibleDepth; ///visible level depth of system
    QPixmap m_pixmap;
    uint    m_ringBreadth;  ///ring breadth
    uint    m_innerRadius;  ///radius of inner circle
    QString m_centerText;

    uint MAP_2MARGIN;
};
}

#endif
