/***************************************************************************
                          filemap.h  -  description
                             -------------------
    begin                : Sun Jul 13 2003
    copyright            : (C) 2003 by Max Howell
    email                : max.howell@methylblue.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FILEMAP_H
#define FILEMAP_H
 
#include <kpixmap.h>

#include "canvas.h"  //for friendships
#include "builder.h" //for friendships


class File;
class QColor;
class Segment;


class FileMap : public KPixmap
{
  public: 
    FileMap();
    virtual ~FileMap();

    void make( const Directory *, bool = false );
    void invalidate( const bool & );
    virtual bool resize( const QRect& );    

    bool isNull() const { return ( m_signature == NULL ); }

    void setGlob( const QString &s ) { m_glob = s; }
    
    friend class Builder;
    friend const Segment *FilelightCanvas::segmentAt( QPoint & ) const;
    friend void FilelightCanvas::paintExplodedLabels( QPainter & ) const;
    friend void FilelightCanvas::refresh( int filth );    

  private:
    void paint( unsigned int = 1 );
    void aaPaint();
    void colorise();
    void setRingBreadth();    

    Chain<Segment> *m_signature; //**** would -> Chain** <- make it an array?
    QRect m_rect;
    unsigned int m_ringBreadth;  //ring breadth
    unsigned int m_innerRadius;  //radius of inner circle
    unsigned int m_visibleDepth; //visible depth of system

    QColor kdeColour[2]; //KDE colours are loaded into this
    
    QCString m_centerText;
    QString  m_glob;

    double deltaRed, deltaGreen, deltaBlue;

    unsigned int MAP_2MARGIN;
};


class Segment //all angles are in 16ths of degrees
{
public:
   Segment( const File *f, unsigned int s, unsigned int l )
    : m_angleStart( s ),
      m_angleSegment( l ),
      m_file( f ),
      m_hasHiddenChildren( false ) { }
   virtual ~Segment() { if( isFake() ) delete m_file; } //created by us in Builder::build()

   const File    *file() const { return m_file; }
   unsigned int  start() const { return m_angleStart; }
   unsigned int length() const { return m_angleSegment; }
   unsigned int    end() const { return m_angleStart + m_angleSegment; }
   const QColor&   pen() const { return m_pen; }
   const QColor& brush() const { return m_brush; }

   bool isFake() const { return ( !m_file->isDir() && m_file->parent() == 0 ); }
   bool hasHiddenChildren() const { return m_hasHiddenChildren; }
   
   bool intersects( unsigned int a ) const { return ( ( a >= start() ) && ( a < end() ) ); }

   friend void FileMap::colorise();
   friend class Builder;
   //friend void Builder::build( const Directory*, unsigned int, unsigned int, unsigned int );

private:
   void setPalette( const QColor &p, const QColor &b ) { m_pen = p; m_brush = b; }

   const unsigned int m_angleStart, m_angleSegment;
   const File* const m_file;
   QColor m_pen, m_brush;
   bool m_hasHiddenChildren;
};

#endif
