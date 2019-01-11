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

#include <QUrl>

#include <KXmlGuiWindow>

class QLabel;

namespace RadialMap {
class Widget;
}
class Folder;

class KSqueezedTextLabel;
class KHistoryComboBox;
class KRecentFilesAction;

class ProgressBox;
class HistoryCollection;

namespace Filelight {

class ScanManager;
class SummaryWidget;

class MainWindow : public KXmlGuiWindow // Maybe use qmainwindow
{
    Q_OBJECT

public:
    MainWindow();

    void scan(const QUrl &u);

Q_SIGNALS:
    void started(); // FIXME: Could be replaced by direct func call once merged with mainwindow
    void completed();
    void canceled(const QString&);
    void setWindowCaption(const QString&);

private Q_SLOTS:
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

    bool openUrl(const QUrl&);
    void configFilelight();
    void rescan();

    void postInit();
    void folderScanCompleted(Folder*);
    void mapChanged(const Folder*);
    void updateURL(const QUrl &);

protected:
    void saveProperties(KConfigGroup&) override;
    void readProperties(const KConfigGroup&) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void setupStatusBar();
    void setupActions();
    bool closeUrl();
    QString prettyUrl() const;
    void showSummary();
    bool start(const QUrl&);

    KSqueezedTextLabel *m_status[2];
    KHistoryComboBox   *m_combo;
    HistoryCollection  *m_histories;
    KRecentFilesAction *m_recentScans;

    QLayout            *m_layout;
    SummaryWidget      *m_summary;
    RadialMap::Widget  *m_map;
    ProgressBox        *m_stateWidget;
    ScanManager        *m_manager;
    QLabel             *m_numberOfFiles;

    bool m_started;


    // KPart Compat helper
public:
    QUrl url() const;
private:
    void setUrl(const QUrl &url);
    QUrl m_url;
};

} // namespace Filelight

#endif
