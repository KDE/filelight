/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
* Copyright 2017  Harald Sitter <sitter@kde.org>
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "part.h"

class KSqueezedTextLabel;
class KHistoryComboBox;
class KRecentFilesAction;

class HistoryCollection;

namespace Filelight {

class MainWindow : public Part // Maybe use qmainwindow
{
    Q_OBJECT

public:
    MainWindow();

    void scan(const QUrl &u) {
        slotScanUrl(u);
    }

private slots:
    void slotUp();
    void slotComboScan();
    void slotScanFolder();
    void slotScanHomeFolder();
    void slotScanRootFolder();
    bool slotScanUrl(const QUrl&);
    bool slotScanPath(const QString&);
    void slotAbortScan();

    void configToolbars();
    void configKeys();

    void scanStarted();
    void scanFailed();
    void scanCompleted();

    void urlAboutToChange();

protected:
    virtual void saveProperties(KConfigGroup&);
    virtual void readProperties(const KConfigGroup&);
    virtual void closeEvent(QCloseEvent *event);

private:
    Filelight::Part *m_part;

    KSqueezedTextLabel *m_status[2];
    KHistoryComboBox   *m_combo;
    HistoryCollection  *m_histories;
    KRecentFilesAction *m_recentScans;

    void setupStatusBar();
    void setupActions();
};

} // namespace Filelight

#endif
