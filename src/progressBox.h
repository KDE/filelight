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

#ifndef PROGRESSBOX_H
#define PROGRESSBOX_H

#include <QLabel>
#include <QTimer>
#include <KColorScheme>

namespace Filelight {
class ScanManager;
class MainWindow;
}

class ProgressBox : public QWidget
{
    Q_OBJECT

public:
    ProgressBox(QWidget *parent, Filelight::MainWindow *mainWindow, Filelight::ScanManager *scanManager);

    void setText(int);

public slots:
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
    int m_textWidth;
    int m_textHeight;
    KColorScheme m_colorScheme;
};

#endif
