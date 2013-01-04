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

#include <QFont>
#include <QFontMetrics>
#include <QPainter>

#include "part/Config.h"
#include "part/fileTree.h"
#include "radialMap.h"
#include "sincos.h"
#include "widget.h"



namespace RadialMap
{
class Label
{
public:
    Label(const RadialMap::Segment *s, int l) : segment(s), lvl(l), angle(segment->start() + (segment->length() / 2)) { }

    bool tooClose(const int otherAngle) const {
        return (angle > otherAngle - LABEL_ANGLE_MARGIN && angle < otherAngle + LABEL_ANGLE_MARGIN);
    }

    const RadialMap::Segment *segment;
    const unsigned int lvl;
    const int angle;

    int x1, y1, x2, y2, x3;
    int tx, ty, tw, th;

    QString qs;
};

int compareAndSortLabels(Label *item1, Label *item2)
{
        //you add 1440 to work round the fact that later you want the circle split vertically
        //and as it is you start at 3 o' clock. It's to do with rightPrevY, stops annoying bug

        int angle1 = (item1)->angle + 1440;
        int angle2 = (item2)->angle + 1440;

        // Also sort by level
        if (angle1 == angle2)            
                return (item1->lvl > item2->lvl);

        if (angle1 > 5760) angle1 -= 5760;
        if (angle2 > 5760) angle2 -= 5760;

        return (angle1 < angle2);

}

void
RadialMap::Widget::paintExplodedLabels(QPainter &paint) const
{
    //we are a friend of RadialMap::Map

    QList<Label*> list;
    unsigned int startLevel = 0;


    //1. Create list of labels  sorted in the order they will be rendered

    if (m_focus && m_focus->file() != m_tree) //separate behavior for selected vs unselected segments
    {
        //don't bother with files
        if (m_focus && m_focus->file() && !m_focus->file()->isFolder())
            return;

        //find the range of levels we will be potentially drawing labels for
        //startLevel is the level above whatever m_focus is in
        for (const Folder *p = (const Folder*)m_focus->file(); p != m_tree; ++startLevel)
            p = p->parent();

        //range=2 means 2 levels to draw labels for

        unsigned int start, end, minAngle;

        start = m_focus->start();
        end = m_focus->end();  //boundary angles
        minAngle = int(m_focus->length() * LABEL_MIN_ANGLE_FACTOR);


#define segment (*it)
#define ring (m_map.m_signature + i)

        //**** Levels should be on a scale starting with 0
        //**** range is a useless parameter
        //**** keep a topblock var which is the lowestLevel OR startLevel for identation purposes
        for (unsigned int i = startLevel; i <= m_map.m_visibleDepth; ++i)
            for (Iterator<Segment> it = ring->iterator(); it != ring->end(); ++it)
                if (segment->start() >= start && segment->end() <= end)
                    if (segment->length() > minAngle) {
                        list.append(new Label(segment, i));
                    }

#undef ring
#undef segment

    } else {

#define ring m_map.m_signature

        for (Iterator<Segment> it = ring->iterator(); it != ring->end(); ++it)
            if ((*it)->length() > 288) {
                list.append(new Label((*it), 0));
                
            }

#undef ring

    }
    qSort(list.begin(), list.end(), compareAndSortLabels);
    
    //2. Check to see if any adjacent labels are too close together
    //   if so, remove it (the least significant labels, since we sort by level too).

    int pos = 0;
    while (pos < list.size() - 1)
    {
        if (list[pos]->tooClose(list[pos+1]->angle))
            list.removeAt(pos+1);
        else
            ++pos;
    }

    //used in next two steps
    bool varySizes;
    //**** should perhaps use doubles
    int  *sizes = new int [ m_map.m_visibleDepth + 1 ]; //**** make sizes an array of floats I think instead (or doubles)

    QList<Label*>::iterator it;

    do
    {
        //3. Calculate font sizes

        {
            //determine current range of levels to draw for
            uint range = 0;

            for (it = list.begin(); it != list.end(); ++it)
            {
                uint lvl = (*it)->lvl;
                if (lvl > range)
                    range = lvl;

                //**** better way would just be to assign if nothing is range
            }

            range -= startLevel; //range 0 means 1 level of labels

            varySizes = Config::varyLabelFontSizes && (range != 0);

            if (varySizes)
            {
                //create an array of font sizes for various levels
                //will exceed normal font pitch automatically if necessary, but not minPitch
                //**** this needs to be checked lots

                //**** what if this is negative (min size gtr than default size)
                uint step = (paint.font().pointSize() - Config::minFontPitch) / range;
                if (step == 0)
                    step = 1;

                for (uint x = range + startLevel, y = Config::minFontPitch; x >= startLevel; y += step, --x)
                    sizes[x] = y;
            }
        }

        //4. determine label co-ordinates

        int x1, y1, x2, y2, x3, tx, ty; //coords
        double sinra, cosra, ra;  //angles

        int cx = m_map.width()  / 2 + m_offset.x();  //centre relative to canvas
        int cy = m_map.height() / 2 + m_offset.y();

        int spacer, preSpacer = int(m_map.m_ringBreadth * 0.5) + m_map.m_innerRadius;
        int fullStrutLength = (m_map.width() - m_map.MAP_2MARGIN) / 2 + LABEL_MAP_SPACER; //full length of a strut from map center

        int prevLeftY  = 0;
        int prevRightY = height();

        bool rightSide;

        QFont font;

        for (it = list.begin(); it != list.end(); ++it)
        {
            //** bear in mind that text is drawn with QPoint param as BOTTOM left corner of text box
            QString string = (*it)->segment->file()->name();
            if (varySizes)
                font.setPointSize(sizes[(*it)->lvl]);
            QFontMetrics fontMetrics(font);
            int fontHeight  = fontMetrics.height(); //used to ensure label texts don't overlap
            int lineSpacing = fontHeight / 4;

            fontHeight += LABEL_TEXT_VMARGIN;

            rightSide = ((*it)->angle < 1440 || (*it)->angle > 4320);

            ra = M_PI/2880 * (*it)->angle; //convert to radians
            sincos(ra, &sinra, &cosra);


            spacer = preSpacer + m_map.m_ringBreadth * (*it)->lvl;

            x1 = cx + (int)(cosra * spacer);
            y1 = cy - (int)(sinra * spacer);
            y2 = y1 - (int)(sinra * (fullStrutLength - spacer));

            if (rightSide) { //righthand side, going upwards
                if (y2 > prevRightY /*- fmh*/) //then it is too low, needs to be drawn higher
                    y2 = prevRightY /*- fmh*/;
            }
            else //lefthand side, going downwards
                if (y2 < prevLeftY/* + fmh*/) //then we're too high, need to be drawn lower
                    y2 = prevLeftY /*+ fmh*/;

            x2 = x1 - int(double(y2 - y1) / tan(ra));
            ty = y2 + lineSpacing;


            if (rightSide) {
                if (x2 > width() || ty < fontHeight || x2 < x1) {
                    //skip this strut
                    //**** don't duplicate this code
                    list.erase(it); //will delete the label and set it to list.current() which _should_ be the next ptr
                    break;
                }

                prevRightY = ty - fontHeight - lineSpacing; //must be after above's "continue"

                string = fontMetrics.elidedText(string, Qt::ElideMiddle, width() - x2);

                x3 = width() - fontMetrics.width(string)
                     - LABEL_HMARGIN //outer margin
                     - LABEL_TEXT_HMARGIN //margin between strut and text
                     //- ((*it)->lvl - startLevel) * LABEL_HMARGIN; //indentation
                     ;
                if (x3 < x2) x3 = x2;
                tx = x3 + LABEL_TEXT_HMARGIN;

            } else {

                if (x2 < 0 || ty > height() || x2 > x1)
                {
                    //skip this strut
                    list.erase(it); //will delete the label and set it to list.current() which _should_ be the next ptr
                    break;
                }

                prevLeftY = ty + fontHeight - lineSpacing;

                string = fontMetrics.elidedText(string, Qt::ElideMiddle, x2);

                //**** needs a little tweaking:

                tx = fontMetrics.width(string) + LABEL_HMARGIN/* + ((*it)->lvl - startLevel) * LABEL_HMARGIN*/;
                if (tx > x2) { //text is too long
                    tx = LABEL_HMARGIN + x2 - tx; //some text will be lost from sight
                    x3 = x2; //no text margin (right side of text here)
                } else {
                    x3 = tx + LABEL_TEXT_HMARGIN;
                    tx = LABEL_HMARGIN /*+ ((*it)->lvl - startLevel) * LABEL_HMARGIN*/;
                }
            }

            (*it)->x1 = x1;
            (*it)->y1 = y1;
            (*it)->x2 = x2;
            (*it)->y2 = y2;
            (*it)->x3 = x3;
            (*it)->tx = tx;
            (*it)->ty = ty;
            (*it)->qs = string;
        }

        //if an element is deleted at this stage, we need to do this whole
        //iteration again, thus the following loop
        //**** in rare case that deleted label was last label in top level
        //     and last in labelList too, this will not work as expected (not critical)

    } while (it != list.end());


    //5. Render labels

    foreach (Label *label, list) {
        if (varySizes) {
            //**** how much overhead in making new QFont each time?
            //     (implicate sharing remember)
            QFont font = paint.font();
            font.setPointSize(sizes[label->lvl]);
            paint.setFont(font);
        }

        paint.drawEllipse(label->x1 - 3, label->y1 - 3, 6, 6); //**** CPU intensive! better to use a pixmap
        paint.drawLine(label->x1,  label->y1, label->x2, label->y2);
        paint.drawLine(label->x2, label->y2, label->x3, label->y2);

        paint.drawText(label->tx, label->ty, label->qs);
    }

    delete [] sizes;
}
}

