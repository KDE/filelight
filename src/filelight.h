/***************************************************************************
                          filelight.h  -  description
                             -------------------
    begin                : Mon May 12 22:38:30 BST 2003
    copyright            : (C) 2003 by Max Howell
    email                : mh9193@bris.ac.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FILELIGHT_H
#define FILELIGHT_H

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

#undef PACKAGE
#undef VERSION
#define PACKAGE "filelight"
#define VERSION "0.6.3"

#include <kmainwindow.h>
#include <kaction.h>

#include "filetree.h"

class QString;
class QTimer;
class QLabel;
class KHistoryCombo;
class KSimpleConfig;
class KAction;
class KRecentFilesAction;
class KPopupMenu;
class KURL;

class FilelightCanvas;
class ScanProgressBox;
class SettingsDlg;
class ScanThread;
class HistoryAction;



class Filelight : public KMainWindow
{
  Q_OBJECT

  public:
    Filelight();
    virtual ~Filelight();

  signals:
    void scanStarted();
    void scanUnrequired( const Directory * );
    void scanFinished( const Directory * );    
    void scanFailed( const QString & );
    
  public slots:
    void slotUpdateInterface( const Directory* );
    void slotUpdateHistories( const QString & );
                            
    void slotClearCache();

    void slotScanUrl( const KURL& );
        
  private slots:
    void slotUp();
    void slotBack();
    void slotForward();
    void slotRescan();

    void startScan( const QString &, bool = false );
    void stopScan();

    void slotComboScan();
            
    void slotScanDirectory();
    void slotScanHomeDirectory();
    void slotScanRootDirectory();

  protected:
    virtual void saveProperties( KConfig * );
    virtual void readProperties( KConfig * );
    virtual void customEvent( QCustomEvent * );
    virtual bool queryExit();
        
  private:
    FilelightCanvas *m_canvas;
    SettingsDlg     *m_settingsDialog;
    QLabel          *m_status[2];
    KSimpleConfig   *m_config;
    bool             m_bool;
    KHistoryCombo   *m_combo;

    HistoryAction      *m_backHistory, *m_forwardHistory;
    KRecentFilesAction *m_recentHistory;

    ScanThread *m_scanThread;

    void setupStatusBar();
    void setupActions();

    const QString validateScan( const QString & );
};

#endif
