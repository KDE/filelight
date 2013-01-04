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

#include "progressBox.h"

#include "scan.h"

#include <KGlobal>
#include <KGlobalSettings>
#include <KIO/Job>
#include <KLocale>

#include <QtGui/QLabel>
#include <QPainter>
#include <QtCore/QDebug>


ProgressBox::ProgressBox(QWidget *parent, QObject *part, Filelight::ScanManager *m)
        : QWidget(parent)
        , m_manager(m)
{
    hide();

    setObjectName(QLatin1String( "ProgressBox" ));

    setFont(KGlobalSettings::fixedFont());
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    setText(999999);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumSize(200, 200);

    connect(&m_timer, SIGNAL(timeout()), SLOT(report()));
    connect(part, SIGNAL(started(KIO::Job*)), SLOT(start()));
    connect(part, SIGNAL(completed()), SLOT(stop()));
    connect(part, SIGNAL(canceled(QString)), SLOT(halt()));
}

void
ProgressBox::start() //slot
{
    m_timer.start(50); //20 times per second - very smooth
    report();
    show();
}

void
ProgressBox::report() //slot
{
    setText(m_manager->files());
    repaint();
}

void
ProgressBox::stop()
{
    m_timer.stop();
}

void
ProgressBox::halt()
{
    // canceled by stop button
    m_timer.stop();
    QTimer::singleShot(2000, this, SLOT(hide()));
}

void
ProgressBox::setText(int files)
{
    m_text = i18np("%1 File", "%1 Files", files);
    m_textWidth = fontMetrics().width(m_text);
    m_textHeight = fontMetrics().height();
}

static QColor getColor(int angle)
{
    angle = abs(angle);
    angle %= 16 * 360;
    int h  = int(angle / 16);
    return QColor::fromHsv(h, 160, 255);
}

void ProgressBox::paintEvent(QPaintEvent*)
{
    QPainter paint(this);
    paint.setRenderHint(QPainter::Antialiasing);
    static int i = 0;
    i+=16;
    
    int angle = 5760-i/2;
    paint.setBrush(getColor(angle));
    paint.drawPie(QRect(15, 15, 175, 175), angle, 300);
    
    angle = 5760+i/1.75;
    paint.setBrush(getColor(angle));
    paint.drawPie(QRect(25, 25, 150, 150), angle, 2000);
    
    paint.setBrush(getColor(i));
    paint.drawPie(QRect(50, 50, 100, 100), i, 200);
    
    angle = 5760-i/3;
    paint.setBrush(getColor(angle));
    paint.drawPie(QRect(75, 75, 50, 50), angle, 2000);
    
    paint.setBrush(QColor(255,255,255,200));
    paint.translate(0.5, 0.5);
    paint.drawRoundedRect(95-m_textWidth/2, 85, m_textWidth+10, m_textHeight+10, 5, 5);
    paint.translate(-0.5, -0.5);
    paint.drawText(100 - m_textWidth/2, 100, m_text);
}


#include "progressBox.moc"
