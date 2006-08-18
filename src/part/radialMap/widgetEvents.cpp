//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include "fileTree.h"
#include "radialMap.h"   //class Segment
#include "widget.h"

#include <cmath>         //::segmentAt()
#include <kcursor.h>     //::mouseMoveEvent()
#include <kiconeffect.h> //::mousePressEvent()
#include <kiconloader.h> //::mousePressEvent()
#include <kio/job.h>     //::mousePressEvent()
#include <klocale.h>
#include <kmessagebox.h> //::mousePressEvent()
#include <kpopupmenu.h>  //::mousePressEvent()
#include <krun.h>        //::mousePressEvent()
#include <kurldrag.h>
#include <qapplication.h>//QApplication::setOverrideCursor()
#include <qclipboard.h>
#include <qpainter.h>
#include <qtimer.h>      //::resizeEvent()



void
RadialMap::Widget::resizeEvent( QResizeEvent* )
{
    if( m_map.resize( rect() ) )
       m_timer.start( 500, true ); //will cause signature to rebuild for new size

    //always do these as they need to be initialised on creation
    m_offset.rx() = (width() - m_map.width()) / 2;
    m_offset.ry() = (height() - m_map.height()) / 2;
}

void
RadialMap::Widget::paintEvent( QPaintEvent* )
{
    //bltBit for some Qt setups will bitBlt _after_ the labels are painted. Which buggers things up!
    //shame as bitBlt is faster, possibly Qt bug? Should report the bug? - seems to be race condition
    //bitBlt( this, m_offset, &m_map );

    QPainter paint( this );

    paint.drawPixmap( m_offset, m_map );

    //vertical strips
    if( m_map.width() < width() )
    {
        paint.eraseRect( 0, 0, m_offset.x(), height() );
        paint.eraseRect( m_map.width() + m_offset.x(), 0, m_offset.x() + 1, height() );
    }
    //horizontal strips
    if( m_map.height() < height() )
    {
        paint.eraseRect( 0, 0, width(), m_offset.y() );
        paint.eraseRect( 0, m_map.height() + m_offset.y(), width(), m_offset.y() + 1 );
    }

    //exploded labels
    if( !m_map.isNull() && !m_timer.isActive() )
       paintExplodedLabels( paint );
}

const RadialMap::Segment*
RadialMap::Widget::segmentAt( QPoint &e ) const
{
    //determine which segment QPoint e is above

    e -= m_offset;

    if( !m_map.m_signature )
       return 0;

    if( e.x() <= m_map.width() && e.y() <= m_map.height() )
    {
        //transform to cartesian coords
        e.rx() -= m_map.width() / 2; //should be an int
        e.ry()  = m_map.height() / 2 - e.y();

        double length = hypot( e.x(), e.y() );

        if( length >= m_map.m_innerRadius ) //not hovering over inner circle
        {
            uint depth  = ((int)length - m_map.m_innerRadius) / m_map.m_ringBreadth;

            if( depth <= m_map.m_visibleDepth ) //**** do earlier since you can //** check not outside of range
            {
                //vector calculation, reduces to simple trigonometry
                //cos angle = (aibi + ajbj) / albl
                //ai = x, bi=1, aj=y, bj=0
                //cos angle = x / (length)

                uint a  = (uint)(acos( (double)e.x() / length ) * 916.736);  //916.7324722 = #radians in circle * 16

                //acos only understands 0-180 degrees
                if( e.y() < 0 ) a = 5760 - a;

                #define ring (m_map.m_signature + depth)
                for( ConstIterator<Segment> it = ring->constIterator(); it != ring->end(); ++it )
                    if( (*it)->intersects( a ) )
                        return *it;
                #undef ring
            }
        }
        else return m_rootSegment; //hovering over inner circle
    }

    return 0;
}

void
RadialMap::Widget::mouseMoveEvent( QMouseEvent *e )
{
   //set m_focus to what we hover over, update UI if it's a new segment

   Segment const * const oldFocus = m_focus;
   QPoint p = e->pos();

   m_focus = segmentAt( p ); //NOTE p is passed by non-const reference

   if( m_focus && m_focus->file() != m_tree )
   {
      if( m_focus != oldFocus ) //if not same as last time
      {
         setCursor( KCursor::handCursor() );
         m_tip->updateTip( m_focus->file(), m_tree );
         emit mouseHover( m_focus->file()->fullPath() );

         //repaint required to update labels now before transparency is generated
         repaint( false );
      }

      m_tip->moveTo( e->globalPos(), *this, ( p.y() < 0 ) ); //updates tooltip psuedo-tranparent background
   }
   else if( oldFocus && oldFocus->file() != m_tree )
   {
      unsetCursor();
      m_tip->hide();
      update();

      emit mouseHover( QString::null );
   }
}

void
RadialMap::Widget::mousePressEvent( QMouseEvent *e )
{
   //m_tip is hidden already by event filter
   //m_focus is set correctly (I've been strict, I assure you it is correct!)

   enum { Konqueror, Konsole, Center, Open, Copy, Delete };

   if (m_focus && !m_focus->isFake())
   {
      const KURL url   = Widget::url( m_focus->file() );
      const bool isDir = m_focus->file()->isDirectory();

      if( e->button() == Qt::RightButton )
      {
         KPopupMenu popup;
         popup.insertTitle( m_focus->file()->fullPath( m_tree ) );

         if (isDir) {
            popup.insertItem( SmallIconSet( "konqueror" ), i18n( "Open &Konqueror Here" ), Konqueror );

            if( url.protocol() == "file" )
               popup.insertItem( SmallIconSet( "konsole" ), i18n( "Open &Konsole Here" ), Konsole );

            if (m_focus->file() != m_tree) {
               popup.insertSeparator();
               popup.insertItem( SmallIconSet( "viewmag" ), i18n( "&Center Map Here" ), Center );
            }
         }
         else
            popup.insertItem( SmallIconSet( "fileopen" ), i18n( "&Open" ), Open );

         popup.insertSeparator();
         popup.insertItem( SmallIconSet( "editcopy" ), i18n( "&Copy to clipboard" ), Copy );

         popup.insertSeparator();
         popup.insertItem( SmallIconSet( "editdelete" ), i18n( "&Delete" ), Delete );

         switch (popup.exec( e->globalPos(), 1 )) {
            case Konqueror:
                //KRun::runCommand will show an error message if there was trouble
                KRun::runCommand( QString( "kfmclient openURL \"%1\"" ).arg( url.url() ) );
                break;

            case Konsole:
                // --workdir only works for local file paths
                KRun::runCommand( QString( "konsole --workdir \"%1\"" ).arg( url.path() ) );
                break;

            case Center:
            case Open:
                goto section_two;

            case Copy:
                QApplication::clipboard()->setData( new KURLDrag( KURL::List( url ) ) );
                break;

            case Delete:
            {
                const KURL url = Widget::url( m_focus->file() );
                const QString message = m_focus->file()->isDirectory()
                        ? i18n( "<qt>The directory at <i>'%1'</i> will be <b>recursively</b> and <b>permanently</b> deleted." )
                        : i18n( "<qt><i>'%1'</i> will be <b>permanently</b> deleted." );
                const int userIntention = KMessageBox::warningContinueCancel(
                        this, message.arg( url.prettyURL() ),
                        QString::null, KGuiItem( i18n("&Delete"), "editdelete" ) );

                if (userIntention == KMessageBox::Continue) {
                    KIO::Job *job = KIO::del( url );
                    job->setWindow( this );
                    connect( job, SIGNAL(result( KIO::Job* )), SLOT(deleteJobFinished( KIO::Job* )) );
                    QApplication::setOverrideCursor( KCursor::workingCursor() );
                }
            }

            default:
                //ensure m_focus is set for new mouse position
                sendFakeMouseEvent();
         }
      }
      else { // not right mouse button

      section_two:
         const QRect rect( e->x() - 20, e->y() - 20, 40, 40 );

         m_tip->hide(); // user expects this

         if (!isDir || e->button() == Qt::MidButton) {
            KIconEffect::visualActivate( this, rect );
            new KRun( url, this, true ); //FIXME see above
         }
         else if (m_focus->file() != m_tree) { // is left click
            KIconEffect::visualActivate( this, rect );
            emit activated( url ); //activate first, this will cause UI to prepare itself
            createFromCache( (Directory *)m_focus->file() );
         }
         else
            emit giveMeTreeFor( url.upURL() );
      }
   }
}

void
RadialMap::Widget::deleteJobFinished( KIO::Job *job )
{
   QApplication::restoreOverrideCursor();
   if( !job->error() )
      invalidate();
   else
      job->showErrorDialog( this );
}

#include "debug.h"
void
RadialMap::Widget::dropEvent( QDropEvent *e )
{
    DEBUG_ANNOUNCE

    KURL::List urls;
    if (KURLDrag::decode( e, urls ) && urls.count())
        emit giveMeTreeFor( urls.first() );
}

void
RadialMap::Widget::dragEnterEvent( QDragEnterEvent *e )
{
    DEBUG_ANNOUNCE

    e->accept( KURLDrag::canDecode( e ) );
}
