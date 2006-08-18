//Author:    Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright: See COPYING file that comes with this distribution

#include "Config.h"
#include "debug.h"
#include "fileTree.h"
#include <kcursor.h>
#include <kiconeffect.h> //MyRadialMap::mousePressEvent()
#include <kiconloader.h>
#include <klocale.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtextstream.h>
#include <qvbox.h>
#include "radialMap/radialMap.h"
#include "radialMap/widget.h"
#include "summaryWidget.h"
#include "summaryWidget.moc"


static Filelight::MapScheme oldScheme;


struct Disk
{
   QString device;
   QString type;
   QString mount;
   QString icon;

   int size;
   int used;
   int free; //NOTE used+avail != size (clustersize!)

   void guessIconName();
};


struct DiskList : QValueList<Disk>
{
   DiskList();
};


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



SummaryWidget::SummaryWidget( QWidget *parent, const char *name )
        : QWidget( parent, name )
{
    qApp->setOverrideCursor( KCursor::waitCursor() );

    setPaletteBackgroundColor( Qt::white );
    (new QGridLayout( this, 1, 2 ))->setAutoAdd( true );

    createDiskMaps();

    qApp->restoreOverrideCursor();
}

SummaryWidget::~SummaryWidget()
{
    Config::scheme = oldScheme;
}

void
SummaryWidget::createDiskMaps()
{
    DiskList disks;

    const QCString free = i18n( "Free" ).local8Bit();
    const QCString used = i18n( "Used" ).local8Bit();

    KIconLoader loader;

    oldScheme = Config::scheme;
    Config::scheme = (Filelight::MapScheme)2000;

    for (DiskList::ConstIterator it = disks.begin(), end = disks.end(); it != end; ++it)
    {
        Disk const &disk = *it;

        if (disk.free == 0 && disk.used == 0)
            continue;

        QWidget *box = new QVBox( this );
        RadialMap::Widget *map = new MyRadialMap( box );

        QString text; QTextOStream( &text )
            << "<img src='" << loader.iconPath( disk.icon, KIcon::Toolbar ) << "'>"
            << " &nbsp;" << disk.mount << " "
            << "<i>(" << disk.device << ")</i>";

        QLabel *label = new QLabel( text, box );
        label->setAlignment( Qt::AlignCenter );
        label->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Maximum );

        box->show(); // will show its children too

        Directory *tree = new Directory( disk.mount.local8Bit() );
        tree->append( free, disk.free );
        tree->append( used, disk.used );

        map->create( tree ); //must be done when visible

        connect( map, SIGNAL(activated( const KURL& )), SIGNAL(activated( const KURL& )) );
    }
}


#if defined(_OS_LINUX_)
#define DF_ARGS       "-kT"
#else
#define DF_ARGS       "-k"
#define NO_FS_TYPE
#endif


DiskList::DiskList()
{
    //FIXME bug prone
    setenv( "LANG", "en_US", 1 );
    setenv( "LC_ALL", "en_US", 1 );
    setenv( "LC_MESSAGES", "en_US", 1 );
    setenv( "LC_TYPE", "en_US", 1 );
    setenv( "LANGUAGE", "en_US", 1 );

    char buffer[4096];
    FILE *df = popen( "env LC_ALL=POSIX df " DF_ARGS, "r" );
    int const N = fread( (void*)buffer, sizeof(char), 4096, df );
    buffer[ N ] = '\0';
    pclose( df );

    QString output = QString::fromLocal8Bit( buffer );
    QTextStream t( &output, IO_ReadOnly );
    QString const BLANK( QChar(' ') );

    while (!t.atEnd()) {
        QString s = t.readLine();
        s = s.simplifyWhiteSpace();

        if (s.isEmpty())
            continue;

        if (s.find( BLANK ) < 0) // devicename was too long, rest in next line
            if (!t.eof()) { // just appends the next line
                QString v = t.readLine();
                s = s.append( v.latin1() );
                s = s.simplifyWhiteSpace();
            }

        Disk disk;
        disk.device = s.left( s.find( BLANK ) );
        s = s.remove( 0, s.find( BLANK ) + 1 );

    #ifndef NO_FS_TYPE
        disk.type = s.left( s.find( BLANK ) );
        s = s.remove( 0, s.find( BLANK ) + 1 );
    #endif

        int n = s.find( BLANK );
        disk.size = s.left( n ).toInt();
        s = s.remove( 0, n + 1 );

        n = s.find( BLANK );
        disk.used = s.left( n ).toInt();
        s = s.remove( 0, n + 1 );

        n = s.find( BLANK );
        disk.free = s.left( n ).toInt();
        s = s.remove( 0, n + 1 );

        s = s.remove( 0, s.find( BLANK ) + 1 );  // delete the capacity 94%
        disk.mount = s;

        disk.guessIconName();

        *this += disk;
    }
}


void
Disk::guessIconName()
{
   if( mount.contains( "cdrom", false ) )       icon = "cdrom";
   else if( device.contains( "cdrom", false ) )  icon = "cdrom";
   else if( mount.contains( "writer", false ) ) icon = "cdwriter";
   else if( device.contains( "writer", false ) ) icon = "cdwriter";
   else if( mount.contains( "mo", false ) )     icon = "mo";
   else if( device.contains( "mo", false ) )     icon = "mo";
   else if( device.contains( "fd", false ) ) {
      if( device.contains( "360", false ) )      icon = "5floppy";
      if( device.contains( "1200", false ) )     icon = "5floppy";
      else
         icon = "3floppy";
   }
   else if( mount.contains( "floppy", false ) ) icon = "3floppy";
   else if( mount.contains( "zip", false ) )    icon = "zip";
   else if( type.contains( "nfs", false ) )        icon = "nfs";
   else
      icon = "hdd";

   icon += /*mounted() ? */"_mount"/* : "_unmount"*/;
}
