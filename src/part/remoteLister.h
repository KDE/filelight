// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#ifndef REMOTE_LISTER_H
#define REMOTE_LISTER_H

#include <kdirlister.h>

namespace Filelight
{
   class RemoteLister : public KDirLister
   {
   Q_OBJECT
   public:
      RemoteLister( const KURL &url, QWidget *parent );
      ~RemoteLister();

   private slots:
      void completed();
      void _completed();
      void canceled();

   private:
      class Store *m_root, *m_store;
  };
}

#endif
