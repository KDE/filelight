//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#ifndef SCAN_H
#define SCAN_H

#include <kurl.h>
#include <qobject.h>

class QThread;
class Directory;
template<class T> class Chain;

namespace Filelight
{
   class ScanManager : public QObject
   {
      Q_OBJECT

      friend class LocalLister;
      friend class RemoteLister;

   public:
      ScanManager( QObject *parent );
      virtual ~ScanManager();

      bool start( const KURL& );
      bool running() const;

      static uint files() { return s_files; }

   public slots:
      bool abort();
      void emptyCache();

   signals:
      void completed( Directory* );
      void aboutToEmptyCache();

   private:
      static bool s_abort;
      static uint s_files;

      KURL m_url;
      QThread *m_thread;
      Chain<Directory> *m_cache;

      virtual void customEvent( QCustomEvent* );
   };
}

#endif
