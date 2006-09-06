// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#ifndef LOCAL_LISTER_H
#define LOCAL_LISTER_H

#include <qthread.h>

class Directory;
template<class T> class Chain;

namespace Filelight
{
   class LocalLister : public QThread
   {
   public:
      LocalLister( const QString &path, Chain<Directory> *cachedTrees, QObject *parent );

      static void readMounts();

   private:
      QString m_path;
      Chain<Directory> *m_trees;
      QObject *m_parent;

   private:
      virtual void run();
      Directory *scan( const QCString&, const QCString& );

   private:
      static QStringList s_local_mounts, s_remote_mounts; //TODO namespace
   };
}

#endif
