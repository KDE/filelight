//Author:    Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright: See COPYING file that comes with this distribution

#include "Config.h"
#include "debug.h"
#include "diskLister.h"
#include "fileTree.h"
#include <kiconeffect.h> //MyRadialMap::mousePressEvent()
#include <kiconloader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtextstream.h>
#include <qvbox.h>
#include "radialMap/radialMap.h"
#include "radialMap/widget.h"
#include "summaryWidget.h"


static Filelight::MapScheme oldScheme;

class MyRadialMap : public RadialMap::Widget
{
public:
   MyRadialMap( QWidget *parent )
      : RadialMap::Widget( parent )
   {}

   virtual void setCursor( const QCursor &c )
   {
      if( focusSegment() && focusSegment()->file()->name() == "Used" )
         RadialMap::Widget::setCursor( c );
      else
         unsetCursor();
   }

   virtual void mousePressEvent( QMouseEvent *e )
   {
      const RadialMap::Segment *segment = focusSegment();

      //we will allow right clicks to the center circle
      if( segment == rootSegment() )
         RadialMap::Widget::mousePressEvent( e );

      //and clicks to the used segment
      else if( segment && segment->file()->name() == "Used" ) {
         const QRect rect( e->x() - 20, e->y() - 20, 40, 40 );
         KIconEffect::visualActivate( this, rect );
         emit activated( url() );
      }
   }
};


namespace Filelight {

SummaryWidget::SummaryWidget( QWidget *parent, const char *name )
   : QWidget( parent, name )
   , m_disks( new DiskList( this ) )
{
   setPaletteBackgroundColor( Qt::white );

   m_disks->readFSTAB();
   m_disks->readDF();

   connect( m_disks, SIGNAL(readDFDone()), SLOT(diskInformationReady()) );

  (new QGridLayout( this, 1, 2 ))->setAutoAdd( true );
}

SummaryWidget::~SummaryWidget()
{
   Config::scheme = oldScheme;
}

void
SummaryWidget::diskInformationReady()
{
   const QCString free = i18n( "Free" ).local8Bit();
   const QCString used = i18n( "Used" ).local8Bit();

   KIconLoader loader;

   oldScheme = Config::scheme;
   Config::scheme = (Filelight::MapScheme)2000;

   for( Disk *disk = m_disks->first(); disk; disk = m_disks->next() )
   {
      QWidget *box = new QVBox( this );
      RadialMap::Widget *map = new MyRadialMap( box );

      QString text; QTextOStream( &text )
         << "<img src='" << loader.iconPath( disk->iconName(), KIcon::Toolbar ) << "'>"
         << " &nbsp;" << disk->mountPoint() << " "
         << "<i>(" << disk->deviceName() << ")</i>";

      QLabel *label = new QLabel( text, box );
      label->setAlignment( Qt::AlignCenter );
      label->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Maximum );

      box->show(); // will show its children too

      Directory *tree = new Directory( disk->mountPoint().local8Bit() );
      tree->append( free, disk->freeKB() );
      tree->append( used, disk->usedKB() );

      map->create( tree ); //must be done when visible

      connect( map, SIGNAL(activated( const KURL& )), SIGNAL(activated( const KURL& )) );
   }

   layout()->addItem( new QSpacerItem( 16, 16, QSizePolicy::Minimum, QSizePolicy::Fixed ) );
}

}

#include "summaryWidget.moc"
