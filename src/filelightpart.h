//
// C++ Interface: filelightpart
//
// Description: 
//
//
// Author: Max Howell <max.howell@methylblue.com>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef FILELIGHTPART_H
#define FILELIGHTPART_H

#include <kparts/browserextension.h>

class FilelightPart;

class FilelightBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT

public:
  FilelightBrowserExtension( FilelightPart *, const char * = 0 );
  ~FilelightBrowserExtension();

protected slots:
/*  
  void selected(TreeMapItem*);
  void contextMenu(TreeMapItem*,const QPoint&);

  void updateActions();
  void refresh();

  void copy() { copySelection( false ); }
  void cut() { copySelection( true ); }
  void trash();
  void del();
  void shred();
  void editMimeType();
*/
};


#include <kparts/part.h>

class QObject;
class QWidget;
class QString;
class QStringList;

class KURL;
class ScanManager;
class KRadialMap;
class KAboutData;

#include "scanmanager.h" //enum

class FilelightPart : public KParts::ReadOnlyPart
{
Q_OBJECT

public:
    FilelightPart( QWidget *, const char *, QObject *, const char *, const QStringList& );
    ~FilelightPart();
    
    virtual bool closeURL();
    virtual bool openFile() { return false; }
    void slotRescan();
    
    static KAboutData* createAboutData();

public slots:
    virtual bool openURL( const KURL & );
        
private slots:
    void scanStarted( const QString & );
    void scanFailed( const QString &, ScanManager::ErrorCode );
    void scanFinished();
    
    void showSettings();

    void updateURL( const KURL & );
    
private:
    FilelightBrowserExtension *m_ext;
    ScanManager *m_manager;
    KRadialMap  *m_map;
};

#endif
