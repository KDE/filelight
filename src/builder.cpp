/***************************************************************************
                          builder.cpp  -  description
                             -------------------
    begin                : Tue Sep 16 2003
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

#include <string.h>
#include <qstring.h>
#include <qregexp.h>
#include <kglobal.h>
#include <klocale.h>

#include "define.h"
#include "builder.h"
#include "filetree.h"
#include "filemap.h"
#include "settings.h"


extern Settings Gsettings;

//**** REMOVE NEED FOR the +1 with MAX_RING_DEPTH uses
//**** add some angle bounds checking (possibly in Segment ctor? can I delete in a ctor?)
//**** this class is a mess
 
Builder::Builder( FileMap *m, const Directory* const dir, bool fast ) :
   m_map( m ),
   m_root( dir ),
   m_minSize( static_cast<unsigned int>((dir->size() * 3) / (PI * m->height() - m->MAP_2MARGIN )) ), //filesize that gives 2px at any depth
   m_depth( &m->m_visibleDepth )
{
  m_signature = new Chain<Segment> [*m_depth + 1];
  
  if( !fast )//|| *m_depth == 0 ) //depth 0 is special case usability-wise //**** WHY?!
  {
    //determine depth rather than use old one
    findVisibleDepth( dir ); //sets m_depth
  }

  m_map->setRingBreadth();
  setLimits( m_map->m_ringBreadth );
  build( dir );

  m_map->m_signature = m_signature;

  delete m_limits;
} 


void
Builder::findVisibleDepth( const Directory* const dir, const unsigned int depth )
{
  //**** because I don't use the same minimumSize criteria as in the visual function
  //     this can lead to incorrect visual representation
  //**** BUT, you can't set those limits until you know m_depth!

  //**** also this function doesn't check to see if anything is actually visible
  //     it just assumes that when it reaches a new level everything in it is visible
  //     automatically. This isn't right especially as there might be no files in the
  //     dir provided to this function!
  
  static unsigned int stopDepth = 0;

  if( dir == m_root )
  {
    stopDepth = *m_depth;
    *m_depth = 0;
  }


  if( *m_depth < depth )
    *m_depth = depth;
  if( *m_depth >= stopDepth )
    return;

  for( ConstIterator<File> it = dir->constIterator(); it != dir->end(); ++it )
  {
    if( (*it)->isDir() ) {
      if( (*it)->size() > m_minSize )
        findVisibleDepth( (Directory *)*it, depth + 1 ); //if no files greater than min size the depth is still recorded
    }
  }
}


void
Builder::setLimits( const unsigned int &b ) //b = breadth?
{
  double size3 = m_root->size() * 3;
  double pi2B   = PI * 2 * b;

  m_limits = new unsigned int [*m_depth + 1];
  
  for( unsigned int d = 0; d <= *m_depth; ++d )
    m_limits[d] = (unsigned int)(size3 / (double)(pi2B * (d + 1))); //min is angle that gives 3px outer diameter for that depth
}


//**** segments currently overlap at edges (i.e. end of first is start of next)
bool
Builder::build( const Directory* const dir, const unsigned int depth, unsigned int a_start, const unsigned int a_end )
{
  //first iteration: dir == m_root

  if( dir->fileCount() == 0 ) //we do fileCount rather than size to avoid chance of divide by zero later
    return false;

  unsigned int hiddenSize = 0, hiddenFileCount = 0;

  for( ConstIterator<File> it = dir->constIterator(); it != dir->end(); ++it )
  {
    if( (*it)->size() > m_limits[depth] )
    {
      unsigned int a_len = (unsigned int)(5760 * ((double)(*it)->size() / (double)m_root->size()));

      Segment *s = new Segment( *it, a_start, a_len );
      
      (m_signature + depth)->append( s );

      if( (*it)->isDir() )
      {
        if( depth == *m_depth )
          s->m_hasHiddenChildren = true;
        else
          s->m_hasHiddenChildren = build( (Directory*)*it, depth + 1, a_start, a_start + a_len ); //recursion
      }

      a_start += a_len; //**** should we add 1?
    }
    else //do always as code is small and if() would just add overhead
    {
      hiddenSize += (*it)->size();
      if( (*it)->isDir() ) //**** considered virtual, but dir wouldn't count itself!
        hiddenFileCount += static_cast<const Directory*>(*it)->fileCount(); //need to add one to count the dir as well

      ++hiddenFileCount;
    }
  }

  if( hiddenFileCount == dir->fileCount() && !Gsettings.showSmallFiles )
    return true;
  else if( ( Gsettings.showSmallFiles && hiddenSize > m_limits[depth] ) ||
           ( depth == 0 && ( hiddenSize > dir->size() / 8 ) ) ) //**** || > size() * 0.75
  {
    //append a segment for unrepresented space
    QString s( i18n( "%1 files ~ %2" ).arg( KGlobal::locale()->formatNumber( hiddenFileCount, 0 ) ).arg( makeHumanReadable( hiddenSize / hiddenFileCount ) ) );
    (m_signature + depth)->append( new Segment( new File( strdup( s.local8Bit() ), hiddenSize ), a_start, a_end - a_start, true ) );
  }

  return false;


}
