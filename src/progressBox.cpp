/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "progressBox.h"

#include "scan.h"
#include "mainWindow.h"

#include <KFormat>
#include <KIO/Job>
#include <KLocalizedString>

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

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    setText(999999, 0);
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
    setText(m_manager->files(), m_manager->totalSize());
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
        const qreal size = qMin(width(), height()) * length[i];
        const QRectF rect(width() / 2 - size / 2, height() / 2 - size / 2, size, size);
        int angle = angleFactor[i] + tick*angleFactor[i];
        QRadialGradient gradient(rect.center(), sin(angle/160.0f) * 100);
        gradient.setColorAt(0, QColor::fromHsv(abs(angle/16) % 360 , 160, 255));
        gradient.setColorAt(1, QColor::fromHsv(abs(angle/16) % 360 , 160, 128));
        QBrush brush(gradient);
        paint.setBrush(brush);
        paint.drawPie(QRectF(rect), angle, aLength[i]);
    }

    paint.translate(0.5, 0.5);
    QRectF textRect(width() / 2 - m_textWidth/2 - 5, width() / 2 - m_textHeight - 5, m_textWidth + 10, m_textHeight + 10);
    paint.fillRect(textRect, m_colorScheme.background(KColorScheme::ActiveBackground).color());
    paint.translate(-0.5, -0.5);
    paint.setPen(m_colorScheme.foreground().color());
    paint.drawText(textRect, Qt::AlignCenter, m_text);
}



