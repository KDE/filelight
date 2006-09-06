// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <kparts/mainwindow.h>

class KSqueezedTextLabel;
class KHistoryCombo;
class KAction;
class KRecentFilesAction;

class ScanProgressBox;
class HistoryCollection;


namespace Filelight {

class Part;

class MainWindow : public KParts::MainWindow
{
  Q_OBJECT

  public:
    MainWindow();

    void scan( const KURL &u ) { slotScanUrl( u ); }

  private slots:
    void slotUp();
    void slotComboScan();
    void slotScanDirectory();
    void slotScanHomeDirectory();
    void slotScanRootDirectory();
    bool slotScanUrl( const KURL& );
    bool slotScanPath( const QString& );
    void slotAbortScan();

    void configToolbars();
    void configKeys();

    void scanStarted();
    void scanFailed();
    void scanCompleted();

    void urlAboutToChange();

  protected:
    virtual void saveProperties( KConfig * );
    virtual void readProperties( KConfig * );
    virtual bool queryExit();

  private:
    Filelight::Part *m_part;

    KSqueezedTextLabel *m_status[2];
    KHistoryCombo      *m_combo;
    HistoryCollection  *m_histories;
    KRecentFilesAction *m_recentScans;

    void setupStatusBar();
    void setupActions();
};

} // namespace Filelight

#endif
