/***************************************************************************
                          filelightcanvas.h  -  description
                             -------------------
    begin                : Sun May 25 2003
    copyright            : (C) 2003 by Max Howell
    email                : mh9193@bris.ac.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FILELIGHTCANVAS_H
#define FILELIGHTCANVAS_H
 
#include <qwidget.h>

//class Map
    #include <kpixmap.h>
    #include <qcstring.h>
    #include <qrect.h>
    #include <qcolor.h>  
//class Segment
    #include <qcolor.h>
//class SegmentTip
    #include <kpixmap.h>
    #include <qstring.h>
//class KRadialMap
    #include <qstring.h>
    #include <qpoint.h>

    
//FIXME NAMESPACE IT!


class Directory;
class File;
class FileMap;
class SegmentTip;
class Segment;
class KMainWindow;
class KActionCollection;
class KAction;
class KURL;
class QStatusBar;
class QString;
class QTimer;
class QPoint;
template <class T> class Chain;
class Label;



class KRadialMap : public QWidget
{
  Q_OBJECT
  
  public:
    KRadialMap( QWidget * = 0, const char * = 0 );
    virtual ~KRadialMap();

    //**** you wish you didn't need this really, although I don't know exactly why
    const QString& path() const { return m_path; }
    bool isValid() const { return (m_tree != NULL); }
    
    friend class Label; //FIXME this needs removing!

  public slots:
    void create( const Directory * );
    void createFromCache( const Directory * );
    void invalidate( const bool & = true );
    
    void refresh( int );

  private slots:
    void slotKonqiHere();
    void slotKonsoleHere();
    void slotRun();
    void slotCenterHere();
    void slotZoomIn();
    void slotZoomOut();    

    void slotResizeTimeout();
    void slotPostMouseEvent();
                                                                                                                                                
signals:
    void activated( const KURL & );
    void invalidated( const KURL & );
    void created( const Directory * );

protected:
    virtual void paintEvent( QPaintEvent * );
    virtual void resizeEvent ( QResizeEvent * );
    virtual void mouseMoveEvent( QMouseEvent * );
    virtual void mousePressEvent( QMouseEvent * );

private:

    class Map;
    class Segment;

    class Builder
    {
    public:
        Builder( Map *, const Directory* const, bool=false );
    
    private:
        void findVisibleDepth( const Directory* const dir, const unsigned int=0 );
        void setLimits( const unsigned int & );
        bool build( const Directory* const, const unsigned int=0, unsigned int=0, const unsigned int=5760 );
    
        Map *m_map;
        const Directory* const m_root;
        const unsigned int m_minSize;
        unsigned int   *m_depth;
        Chain<Segment> *m_signature; //**** try ** here
        unsigned int   *m_limits;
    };

    class Map : public KPixmap
    {
    public:
        Map();
        ~Map();
            
        void make( const Directory *, bool = false );
        bool resize( const QRect& );
        
        bool isNull() const { return ( m_signature == NULL ); }
        void invalidate( const bool );
        
        friend class Builder;
        friend class KRadialMap;
        
    private:
        void paint( unsigned int = 1 );
        void aaPaint();
        void colorise();
        void setRingBreadth();    
    
        Chain<Segment> *m_signature;
        Segment        *m_rootSegment;
        QRect           m_rect;
        unsigned int    m_ringBreadth;  //ring breadth
        unsigned int    m_innerRadius;  //radius of inner circle
        unsigned int    m_visibleDepth; //visible depth of system
    
        QColor kdeColour[2]; //KDE colours are loaded into this
        
        QCString m_centerText;
    
        double deltaRed, deltaGreen, deltaBlue;
    
        unsigned int MAP_2MARGIN;
        
    };
    
    class Segment //all angles are in 16ths of degrees
    {
    public:
        Segment( const File *f, unsigned int s, unsigned int l, bool isFake = false )
          : m_angleStart( s )
          , m_angleSegment( l )
          , m_file( f )
          , m_hasHiddenChildren( false )
          , m_fake( isFake ) {}
        ~Segment() { if( isFake() ) delete m_file; }//created by us in Builder::build()
    
        const File    *file() const { return m_file; }
        unsigned int  start() const { return m_angleStart; }
        unsigned int length() const { return m_angleSegment; }
        unsigned int    end() const { return m_angleStart + m_angleSegment; }
        const QColor&   pen() const { return m_pen; }
        const QColor& brush() const { return m_brush; }
        
        bool isFake() const { return m_fake; }
        bool hasHiddenChildren() const { return m_hasHiddenChildren; }
        
        bool intersects( unsigned int a ) const { return ( ( a >= start() ) && ( a < end() ) ); }
        
        friend void Map::colorise();
        friend class Builder;
        
    private:
        void setPalette( const QColor &p, const QColor &b ) { m_pen = p; m_brush = b; }
        
        const unsigned int m_angleStart, m_angleSegment;
        const File* const m_file;
        QColor m_pen, m_brush;
        bool m_hasHiddenChildren;
        const bool m_fake;
    };
  
    class SegmentTip : public QWidget
    {
    public: 
        SegmentTip( unsigned int );
        virtual ~SegmentTip();

        void update();

        void updateTip( const File *, const Directory * );
        void moveto( QPoint, const QWidget&, bool );

    private:
        virtual bool eventFilter( QObject *, QEvent * );
        virtual void showEvent( QShowEvent * );
        virtual void hideEvent( QHideEvent * );

        unsigned int m_cursorHeight;
        KPixmap m_pixmap;
        QString m_text;
        QTimer *m_timer;    
    };  

   
    const Segment *segmentAt( QPoint & ) const;
    void paintExplodedLabels( QPainter & ) const;

    const Directory *m_tree;
    const Segment   *m_focus;
    QPoint      m_offset;
    QString     m_path;

    QTimer     *m_timer;
    QStatusBar *m_status;
    Map         m_map;
    SegmentTip  m_tip;

    KActionCollection *m_actionJug;

    KAction *m_actKonqi;
    KAction *m_actKonsole;
    KAction *m_actCenter;
    KAction *m_actRun;
};


#ifndef PI
#define PI 3.141592653589793
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#define LABEL_MAP_SPACER 7    //distance from map to horizontal strut of labels
#define LABEL_HMARGIN 10
#define LABEL_TEXT_HMARGIN 5
#define LABEL_TEXT_VMARGIN 0
#define LABEL_ANGLE_MARGIN 32 //in 16ths of degree
#define LABEL_MIN_ANGLE_FACTOR 0.05
#define LABEL_MAX_CHARS 30

#define MIN_RING_BREADTH 20
#define MAX_RING_BREADTH 60
#define DEFAULT_RING_DEPTH 4 //first level = 0
#define MIN_RING_DEPTH 0


//factor for filesizes in scan, i.e. show in kB, MB, GB or TB (TB only possible on 64bit systems)
const unsigned int UNIT_DENOMINATOR[4] = { 1, 1024, 1048576, 1073741824 };
const char UNIT_PREFIX[4] = { 'k', 'M', 'G', 'T' };

enum UnitPrefix { kilo, mega, giga, tera };

QString fullPath( const File *, const Directory * const = 0 );
QString makeHumanReadable( unsigned int, UnitPrefix = mega );

#endif
