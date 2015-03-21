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

#include <KColorScheme>
#include <KIO/Job>
#include <KLocalizedString>

#include <QLabel>
#include <QPainter>
#include <QDebug>

#include <QFontDatabase>

#include <math.h>


ProgressBox::ProgressBox(QWidget *parent, QObject *part, Filelight::ScanManager *m)
        : QWidget(parent)
        , m_manager(m)
{
    hide();

    setObjectName(QStringLiteral( "ProgressBox" ));

    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
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
    update(); //repaint();
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

#define PIECES_NUM 4
static const float angleFactor[] = { -0.75, 0.5, 1.0, -0.3 };
static const int length[] = { 30, 40, 50, 60 };
static const int angleOffset[] = { 5760, 0, 0, -5760 };
static const int aLength[] = { 300, 2000, 200, 2000 };

void ProgressBox::paintEvent(QPaintEvent*)
{
    KColorScheme view = KColorScheme(QPalette::Active, KColorScheme::Tooltip);

    QPainter paint(this);
    paint.setRenderHint(QPainter::Antialiasing);
    static int tick = 0;
    tick+=16;

    for (int i=0; i<PIECES_NUM; i++) {
        const QRect rect(length[i]/2, length[i]/2, 200- length[i], 200-length[i]);
        int angle = angleFactor[i] + tick*angleFactor[i];
        QRadialGradient gradient(rect.center(), sin(angle/160.0f) * 100);
        gradient.setColorAt(0, QColor::fromHsv(abs(angle/16) % 360 , 160, 255));
        gradient.setColorAt(1, QColor::fromHsv(abs(angle/16) % 360 , 160, 128));
        QBrush brush(gradient);
        paint.setBrush(brush);
        paint.drawPie(QRect(rect), angle, aLength[i]);
    }

    paint.setBrush(view.background(KColorScheme::ActiveBackground));
    paint.setPen(view.foreground().color());
    paint.translate(0.5, 0.5);
    paint.drawRoundedRect(95-m_textWidth/2, 85, m_textWidth+10, m_textHeight+10, 5, 5);
    paint.translate(-0.5, -0.5);
    paint.drawText(100 - m_textWidth/2, 100, m_text);
}



