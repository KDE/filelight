// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

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
      explicit ScanManager( QObject *parent );
      ~ScanManager();

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
