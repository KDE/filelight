// Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
// Copyright: See COPYING file that comes with this distribution

#ifndef FILELIGHTPART_H
#define FILELIGHTPART_H

#include <kparts/browserextension.h>
#include <kparts/statusbarextension.h>
#include <kparts/part.h>
#include <kurl.h>

class KAboutData;
using KParts::StatusBarExtension;
namespace RadialMap { class Widget; }
class Directory;


namespace Filelight
{
   class Part;

   class BrowserExtension : public KParts::BrowserExtension
   {
   Q_OBJECT

   public:
      BrowserExtension( Part*, const char * = 0 );

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


   class Part : public KParts::ReadOnlyPart
   {
   Q_OBJECT

   public:
      Part( QWidget *, const char *, QObject *, const char *, const QStringList& );

      virtual bool openFile() { return false; } //pure virtual in base class
      virtual bool closeURL();

      QString prettyURL() const { return m_url.protocol() == "file" ? m_url.path() : m_url.prettyURL(); }

      static KAboutData *createAboutData();

   public slots:
      virtual bool openURL( const KURL& );
      void configFilelight();
      void rescan();

   private slots:
      void postInit();
      void scanCompleted( Directory* );
      void mapChanged( const Directory* );

   private:
      KStatusBar *statusBar() { return m_statusbar->statusBar(); }

      BrowserExtension   *m_ext;
      StatusBarExtension *m_statusbar;
      RadialMap::Widget  *m_map;
      class ScanManager  *m_manager;

   private:
      bool start( const KURL& );

   private slots:
      void updateURL( const KURL & );
   };
}

#endif
