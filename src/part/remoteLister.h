//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#ifndef REMOTELISTER_H
#define REMOTELISTER_H

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
