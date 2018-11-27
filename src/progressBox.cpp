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
#include "mainWindow.h"

#include <KColorScheme>
#include <KIO/Job>
#include <KLocalizedString>

#include <QLabel>
#include <QPainter>

#include <QFontDatabase>

#include <math.h>

ProgressBox::ProgressBox(QWidget *parent, Filelight::MainWindow *mainWindow, Filelight::ScanManager *scanManager)
        : QWidget(parent)
        , m_manager(scanManager)
        , m_colorScheme(QPalette::Active, KColorScheme::Tooltip)
{
    hide();

    setObjectName(QStringLiteral( "ProgressBox" ));

    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    setText(999999);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumSize(300, 300);

    connect(&m_timer, &QTimer::timeout, this, &ProgressBox::report);
    connect(mainWindow, &Filelight::MainWindow::started, this, &ProgressBox::start);
    connect(mainWindow, &Filelight::MainWindow::completed, this, &ProgressBox::stop);
    connect(mainWindow, &Filelight::MainWindow::canceled, this, &ProgressBox::halt);
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
static const float angleFactor[] = { -0.25, 0.9, -1.0, 0.3 };
static const float length[] = { 1.0, 1.0, 1.0, 1.0 };
static const int aLength[] = { 2000, 2000, 2000, 2000 };

void ProgressBox::paintEvent(QPaintEvent*)
{

    QPainter paint(this);
    paint.setPen(Qt::transparent);
    paint.setRenderHint(QPainter::Antialiasing);
    static int tick = 0;
    tick+=16;

    for (int i=0; i<PIECES_NUM; i++) {
        const int size = qMin(width(), height()) * length[i];
        const QRect rect(width() / 2 - size / 2, height() / 2 - size / 2, size, size);
        int angle = angleFactor[i] + tick*angleFactor[i];
        QRadialGradient gradient(rect.center(), sin(angle/160.0f) * 100);
        gradient.setColorAt(0, QColor::fromHsv(abs(angle/16) % 360 , 160, 255));
        gradient.setColorAt(1, QColor::fromHsv(abs(angle/16) % 360 , 160, 128));
        QBrush brush(gradient);
        paint.setBrush(brush);
        paint.drawPie(QRect(rect), angle, aLength[i]);
    }

    paint.translate(0.5, 0.5);
    QRectF textRect(width() / 2 - m_textWidth/2 - 5, width() / 2 - m_textHeight - 5, m_textWidth + 10, m_textHeight + 10);
    paint.fillRect(textRect, m_colorScheme.background(KColorScheme::ActiveBackground).color());
    paint.translate(-0.5, -0.5);
    paint.setPen(m_colorScheme.foreground().color());
    paint.drawText(textRect, Qt::AlignCenter, m_text);
}



