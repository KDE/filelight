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
#include "scanmanager.h" //**** unless you remember how to declare the enum

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
class ScanManager;
class HistoryCollection;
class Directory;


class Filelight : public KMainWindow
{
  Q_OBJECT

  public:
    Filelight();
    virtual ~Filelight();

  public slots:
    void slotScanUrl( const KURL & ); //needed by main.cpp
    
  private slots:
    void slotUp();
    void slotRescan();
    void slotComboScan();
    void slotScanDirectory();
    void slotScanHomeDirectory();
    void slotScanRootDirectory();

    void editToolbars();
    void slotNewToolbarConfig();

    void scanStarted( const QString & );
    void scanFailed( const QString &, ScanManager::ErrorCode );
    void newMapCreated( const Directory * );    

  protected:
    virtual void saveProperties( KConfig * );
    virtual void readProperties( KConfig * );
    virtual bool queryExit();
        
  private:
    FilelightCanvas *m_canvas;
    SettingsDlg     *m_settingsDialog;
    QLabel          *m_status[2];
    KSimpleConfig   *m_config;
    KHistoryCombo   *m_combo;
    ScanManager     *m_manager;
    
    HistoryCollection *m_histories;
    KRecentFilesAction *m_recentHistory;

    void setupStatusBar();
    void setupActions();
};

#endif
