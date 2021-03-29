/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "progressBox.h"

#include "scan.h"
#include "mainWindow.h"
#include "radialMap/radialMap.h"

#include <KFormat>
#include <KIO/Job>
#include <KLocalizedString>

#include <QPainter>

#include <QFontDatabase>

#include <math.h>
#include <limits>

ProgressBox::ProgressBox(QWidget *parent, Filelight::MainWindow *mainWindow, Filelight::ScanManager *scanManager)
        : QWidget(parent)
        , m_manager(scanManager)
        , m_colorScheme(QPalette::Active, KColorScheme::Tooltip)
{
    hide();

    setObjectName(QStringLiteral( "ProgressBox" ));

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setText(std::numeric_limits<int>::max(), std::numeric_limits<size_t>::max());

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    const int textSize = qMax(m_textWidth + LABEL_TEXT_HMARGIN, m_textHeight + LABEL_TEXT_VMARGIN);
    setMinimumSize(qMax(textSize, 300), qMax(textSize, 300));

    connect(&m_timer, &QTimer::timeout, this, &ProgressBox::report);
    connect(mainWindow, &Filelight::MainWindow::started, this, &ProgressBox::start);
    connect(mainWindow, &Filelight::MainWindow::completed, this, &ProgressBox::stop);
    connect(mainWindow, &Filelight::MainWindow::canceled, this, &ProgressBox::halt);
}

void
ProgressBox::start() //slot
{
    // 60 fps, because Qt has that hardcoded everywhere and I'm not in the mood to
    // manually get the display FPS on all platforms.
    m_timer.start(16);
    report();
    show();
    m_tick = 0;
}

void
ProgressBox::report() //slot
{
    setText(m_manager->files(), m_manager->totalSize());
    update();
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
    QTimer::singleShot(2000, this, &QWidget::hide);
}

void
ProgressBox::setText(int files, size_t totalSize)
{
    m_text = i18ncp("Scanned number of files and size so far", "%1 File, %2", "%1 Files, %2", files, KFormat().formatByteSize(totalSize));
    m_textWidth = fontMetrics().boundingRect(m_text).width();
    m_textHeight = fontMetrics().height();
}

#define PIECES_NUM 4
static const float angleFactor[] = { -0.15, 0.25, -0.3, 0.1 };
static const float length[] = { 1.0, 1.0, 0.75, 0.9 };
static const int aLength[] = { 2000, 2000, 2000, 2000 };

void ProgressBox::paintEvent(QPaintEvent*)
{

    QPainter paint(this);
    paint.setPen(Qt::transparent);
    paint.setRenderHint(QPainter::Antialiasing);
    m_tick += 16;

    for (int i=0; i<PIECES_NUM; i++) {
        const qreal size = qMin(width(), height()) * length[i];
        const QRectF rect(width() / 2 - size / 2, height() / 2 - size / 2, size, size);
        int angle = angleFactor[i] + m_tick * angleFactor[i];
        QRadialGradient gradient(rect.center(), sin(angle/160.0f) * 100);
        gradient.setColorAt(0, QColor::fromHsv(abs(angle/16) % 360 , 160, 255));
        gradient.setColorAt(1, QColor::fromHsv(abs(angle/16) % 360 , 160, 128));
        QBrush brush(gradient);
        paint.setBrush(brush);
        paint.drawPie(QRectF(rect), angle, qMin(m_tick/1600., 1.0) * aLength[i]);
    }

    paint.translate(0.5, 0.5);
    QRectF textRect(width() / 2 - m_textWidth/2 - 5, width() / 2 - m_textHeight - 5, m_textWidth + 10, m_textHeight + 10);
    paint.fillRect(textRect, m_colorScheme.background(KColorScheme::ActiveBackground).color());
    paint.translate(-0.5, -0.5);
    paint.setPen(m_colorScheme.foreground().color());
    paint.drawText(textRect, Qt::AlignCenter, m_text);
}



