//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include <debug.h>
#include "fileTree.h"
#include <qapplication.h>
#include <qtimer.h>
#include <qvaluelist.h>
#include "remoteLister.h"
#include "scan.h"

namespace Filelight
{
   //you need to use a single DirLister
   //one per directory breaks KIO (seemingly) and also uses un-godly amounts of memory!

   //TODO delete all this stuff!

   struct Store {

      typedef QValueList<Store*> List;

      /// location of the directory
      const KURL url;
      /// the directory on which we are operating
      Directory *directory;
      /// so we can reference the parent store
      Store *parent;
      /// directories in this directory that need to be scanned before we can propagate()
      List stores;

      Store()
         : directory( 0 ), parent( 0 ) {}
      Store( const KURL &u, const QString &name, Store *s )
         : url( u ), directory( new Directory( name.local8Bit() + '/' ) ), parent( s ) {}


      Store*
      propagate()
      {
         /// returns the next store available for scanning

         debug() << "propagate: " << url << endl;

         if( parent ) {
            parent->directory->append( directory );
            if( parent->stores.isEmpty() ) {
               return parent->propagate();
            }
            else
               return parent;
         }

         //we reached the root, let's get our next directory scanned
         return this;
      }

   private:
      Store( Store& );
      Store &operator=( const Store& );
   };


   RemoteLister::RemoteLister( const KURL &url, QWidget *parent )
      : KDirLister( true /*don't fetch mimetypes*/ )
      , m_root( new Store( url, url.url(), 0 ) )
      , m_store( m_root )
   {
      setAutoUpdate( false ); //don't use KDirWatchers
      setShowingDotFiles( true ); //stupid KDirLister API function names
      setMainWindow( parent );

      //use SIGNAL(result(KIO::Job*)) instead and then use Job::error()
      connect( this, SIGNAL(completed()), SLOT(completed()) );
      connect( this, SIGNAL(canceled()), SLOT(canceled()) );

      //we do this non-recursively - it is the only way!
      openURL( url );
   }

   RemoteLister::~RemoteLister()
   {
      Directory *tree = isFinished() ? m_store->directory : 0;

      QCustomEvent *e = new QCustomEvent( 1000 );
      e->setData( tree );
      QApplication::postEvent( parent(), e );

      delete m_root;
   }

   void
   RemoteLister::completed()
   {
      debug() << "completed: " << url().prettyURL() << endl;

      //as usual KDE documentation didn't suggest I needed to do this at all
      //I had to figure it out myself
      // -- avoid crash
      QTimer::singleShot( 0, this, SLOT(_completed()) );
   }

   void
   RemoteLister::canceled()
   {
      debug() << "canceled: " << url().prettyURL() << endl;

      QTimer::singleShot( 0, this, SLOT(_completed()) );
   }

   void
   RemoteLister::_completed()
   {
      //m_directory is set to the directory we should operate on

      KFileItemList items = KDirLister::items();
      for( KFileItemList::ConstIterator it = items.begin(), end = items.end(); it != end; ++it )
      {
         if( (*it)->isDir() )
            m_store->stores += new Store( (*it)->url(), (*it)->name(), m_store );
         else
            m_store->directory->append( (*it)->name().local8Bit(), (*it)->size() / 1024 );

         ScanManager::s_files++;
      }


      if( m_store->stores.isEmpty() )
         //no directories to scan, so we need to append ourselves to the parent directory
         //propagate() will return the next ancestor that has stores left to be scanned, or root if we are done
         m_store = m_store->propagate();

      if( !m_store->stores.isEmpty() )
      {
         Store::List::Iterator first = m_store->stores.begin();
         const KURL url( (*first)->url );
         Store *currentStore = m_store;

         //we should operate with this store next time this function is called
         m_store = *first;

         //we don't want to handle this store again
         currentStore->stores.remove( first );

         //this returns _immediately_
         debug() << "scanning: " << url << endl;
         openURL( url );
      }
      else {

         debug() << "I think we're done\n";

         Q_ASSERT( m_root == m_store );

         delete this;
      }
   }
}

#include "remoteLister.moc"
