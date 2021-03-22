/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef PROGRESSBOX_H
#define PROGRESSBOX_H

#include <QTimer>
#include <KColorScheme>
#include <QWidget>

namespace Filelight {
class ScanManager;
class MainWindow;
}

class ProgressBox : public QWidget
{
    Q_OBJECT

public:
    ProgressBox(QWidget *parent, Filelight::MainWindow *mainWindow, Filelight::ScanManager *scanManager);

    void setText(int files, size_t totalSize);

public Q_SLOTS:
    void start();
    void report();
    void stop();
    void halt();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTimer m_timer;
    Filelight::ScanManager* m_manager;
    QString m_text;
    int m_textWidth = 0;
    int m_textHeight = 0;
    int m_tick = 0;
    KColorScheme m_colorScheme;
};

#endif
