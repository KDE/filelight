//Author:    Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright: See COPYING file that comes with this distribution

#ifndef WIDGET_H
#define WIDGET_H

#include <kurl.h>
#include <qtimer.h>
#include "segmentTip.h"

template <class T> class Chain;
class Directory;
class File;
namespace KIO { class Job; }
class KURL;

namespace RadialMap
{
    class Segment;

    class Map : public KPixmap
    {
    public:
        Map();
        ~Map();

        void make( const Directory *, bool = false );
        bool resize( const QRect& );

        bool isNull() const { return ( m_signature == 0 ); }
        void invalidate( const bool );

        friend class Builder;
        friend class Widget;

    private:
        void paint( uint = 1 );
        void aaPaint();
        void colorise();
        void setRingBreadth();

        Chain<Segment> *m_signature;

        QRect   m_rect;
        uint    m_ringBreadth;  ///ring breadth
        uint    m_innerRadius;  ///radius of inner circle
        uint    m_visibleDepth; ///visible level depth of system
        QString m_centerText;

        uint MAP_2MARGIN;
    };

    class Widget : public QWidget
    {
    Q_OBJECT

    public:
        Widget( QWidget* = 0, const char* = 0 );

        QString path() const;
        KURL url( File const * const = 0 ) const;

        bool isValid() const { return m_tree != 0; }

        friend class Label; //FIXME badness

    public slots:
        void zoomIn();
        void zoomOut();
        void create( const Directory* );
        void invalidate( const bool = true );
        void refresh( int );

    private slots:
        void resizeTimeout();
        void sendFakeMouseEvent();
        void deleteJobFinished( KIO::Job* );
        void createFromCache( const Directory* );

    signals:
        void activated( const KURL& );
        void invalidated( const KURL& );
        void created( const Directory* );
        void mouseHover( const QString& );

    protected:
        virtual void paintEvent( QPaintEvent* );
        virtual void resizeEvent( QResizeEvent* );
        virtual void mouseMoveEvent( QMouseEvent* );
        virtual void mousePressEvent( QMouseEvent* );

    protected:
        const Segment *segmentAt( QPoint& ) const; //FIXME const reference for a library others can use
        const Segment *rootSegment() const { return m_rootSegment; } ///never == 0
        const Segment *focusSegment() const { return m_focus; } ///0 == nothing in focus

    private:
        void paintExplodedLabels( QPainter& ) const;

        const Directory *m_tree;
        const Segment   *m_focus;
        QPoint           m_offset;
        QTimer           m_timer;
        Map              m_map;
        SegmentTip       m_tip;
        Segment         *m_rootSegment;
    };
}

#endif
