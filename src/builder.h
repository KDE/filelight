/***************************************************************************
                          builder.h  -  description
                             -------------------
    begin                : Tue Sep 16 2003
    copyright            : (C) 2003 by Max Howell
    email                : max.howell@methylblue.com
 ***************************************************************************/

//This class is a convenience class for building signatures.
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef BUILDER_H
#define BUILDER_H

#include "define.h"
#include "filemap.h"
#include "filetree.h"

class Builder
{
public:
    Builder( FileMap *, const Directory* const, bool=false );

private:
    void findVisibleDepth( const Directory* const dir, const unsigned int=0 );
    void setLimits( const unsigned int & );
    bool build( const Directory* const, const unsigned int=0, unsigned int=0, const unsigned int=5760 );

    FileMap* const m_map;
    const Directory* const m_root;
    const unsigned int m_minSize;
    unsigned int   *m_depth;
    Chain<Segment> *m_signature; //**** try ** here
    unsigned int    m_limits[MAX_MAX_RING_DEPTH];
};

#endif
