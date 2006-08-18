//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include "fileTree.h"
#include "segmentTip.h"

#include <cstdlib>
#include <kapplication.h>    //installing eventFilters
#include <kglobal.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kpixmapeffect.h>
#include <qpainter.h>
#include <qtooltip.h>        //for its palette



namespace RadialMap {


bool isBackingStoreActive()
{
    // # xdpyinfo | grep backing
    // options:    backing-store YES, save-unders YES

    char buffer[4096];
    FILE *xdpyinfo = popen( "xdpyinfo", "r" );
    int const N = fread( (void*)buffer, sizeof(char), 4096, xdpyinfo );
    buffer[ N ] = '\0';
    pclose( xdpyinfo );

    return QString::fromLocal8Bit( buffer ).contains( "backing-store YES" );
}


SegmentTip::SegmentTip( uint h )
        : QWidget( 0, 0, WNoAutoErase | WStyle_Customize | WStyle_NoBorder | WStyle_Tool | WStyle_StaysOnTop | WX11BypassWM )
        , m_cursorHeight( -h )
        , m_backing_store( isBackingStoreActive() )
{
    setBackgroundMode( Qt::NoBackground );
}

void
SegmentTip::moveTo( QPoint p, const QWidget &canvas, bool placeAbove )
{
    //**** this function is very slow and seems to be visibly influenced by operations like mapFromGlobal() (who knows why!)
    //  ** so any improvements are much desired

    //TODO uints could improve the class
    p.rx() -= rect().center().x();
    p.ry() -= (placeAbove ? 8 + height() : m_cursorHeight - 8);

    const QRect screen = KGlobalSettings::desktopGeometry( parentWidget() );

    const int x  = p.x();
    const int y  = p.y();
    const int x2 = x + width();
    const int y2 = y + height(); //how's it ever gunna get below screen height?! (well you never know I spose)
    const int sw = screen.width();
    const int sh = screen.height();

    if( x  < 0  ) p.setX( 0 );
    if( y  < 0  ) p.setY( 0 );
    if( x2 > sw ) p.rx() -= x2 - sw;
    if( y2 > sh ) p.ry() -= y2 - sh;


    //I'm using this QPoint to determine where to offset the bitBlt in m_pixmap
    QPoint offset = canvas.mapToGlobal( QPoint() ) - p;
    if( offset.x() < 0 ) offset.setX( 0 );
    if( offset.y() < 0 ) offset.setY( 0 );


    const QRect alphaMaskRect( canvas.mapFromGlobal( p ), size() );
    const QRect intersection( alphaMaskRect.intersect( canvas.rect() ) );

    m_pixmap.resize( size() ); //move to updateTip once you are sure it can never be null
    bitBlt( &m_pixmap, offset, &canvas, intersection, Qt::CopyROP );

    QColor const c = QToolTip::palette().color( QPalette::Active, QColorGroup::Background );
    if (!m_backing_store)
        m_pixmap.fill( c );

    QPainter paint( &m_pixmap );
    paint.setPen( Qt::black );
    paint.setBrush( Qt::NoBrush );
    paint.drawRect( rect() );
    paint.end();

    if (m_backing_store)
        m_pixmap = KPixmapEffect::fade( m_pixmap, 0.6, c );

    paint.begin( &m_pixmap );
    paint.drawText( rect(), AlignCenter, m_text );
    paint.end();

    p += screen.topLeft(); //for Xinerama users

    move( x, y );
    show();
    update();
}

void
SegmentTip::updateTip( const File* const file, const Directory* const root )
{
    const QString s1  = file->fullPath( root );
    QString s2        = file->humanReadableSize();
    KLocale *loc      = KGlobal::locale();
    const uint MARGIN = 3;
    const uint pc     = 100 * file->size() / root->size();
    uint maxw         = 0;
    uint h            = fontMetrics().height()*2 + 2*MARGIN;

    if( pc > 0 ) s2 += QString( " (%1%)" ).arg( loc->formatNumber( pc, 0 ) );

    m_text  = s1;
    m_text += '\n';
    m_text += s2;

    if( file->isDirectory() )
    {
        double files  = static_cast<const Directory*>(file)->children();
        const uint pc = uint((100 * files) / (double)root->children());
        QString s3    = i18n( "Files: %1" ).arg( loc->formatNumber( files, 0 ) );

        if( pc > 0 ) s3 += QString( " (%1%)" ).arg( loc->formatNumber( pc, 0 ) );

        maxw    = fontMetrics().width( s3 );
        h      += fontMetrics().height();
        m_text += '\n';
        m_text += s3;
    }

    uint
    w = fontMetrics().width( s1 ); if( w > maxw ) maxw = w;
    w = fontMetrics().width( s2 ); if( w > maxw ) maxw = w;

    resize( maxw + 2 * MARGIN, h );
}

bool
SegmentTip::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::Show:
        kapp->installEventFilter( this );
        break;
    case QEvent::Hide:
        kapp->removeEventFilter( this );
        break;
    case QEvent::Paint:
    {
        //QPainter( this ).drawPixmap( 0, 0, m_pixmap );
        bitBlt( this, 0, 0, &m_pixmap );
        return true;
    }
    default:
        ;
    }

    return false/*QWidget::event( e )*/;
}

bool
SegmentTip::eventFilter( QObject*, QEvent *e )
{
    switch ( e->type() )
    {
    case QEvent::Leave:
//     case QEvent::MouseButtonPress:
//     case QEvent::MouseButtonRelease:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::Wheel:
        hide(); //FALL THROUGH
    default:
        return false; //allow this event to passed to target
    }
}

} //namespace RadialMap
