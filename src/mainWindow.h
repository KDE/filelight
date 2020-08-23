/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
* SPDX-FileCopyrightText: 2017 Harald Sitter <sitter@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    void slotSaveSvg();
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
    void rescanSingleDir(const QUrl &);

protected:
    void saveProperties(KConfigGroup&) override;
    void readProperties(const KConfigGroup&) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void setupActions();
    bool closeUrl();
    QString prettyUrl(const QUrl &url) const;
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
