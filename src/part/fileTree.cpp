//Author: Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright: See COPYING file that comes with this distribution

#include "fileTree.h"
#include <kglobal.h>
#include <klocale.h>
#include <qfile.h>


//static definitions
const uint File::DENOMINATOR[4] = { 1<<0, 1<<10, 1<<20, 1<<30 };
static const char PREFIX[4]   = { 'K', 'M', 'G', 'T' };


QString
File::fullPath( const Directory *root /*= 0*/ ) const
{
   QString path;

   if( root == this )
      root = 0; //prevent returning empty string when there is something we could return

   for( const Directory *d = (Directory*)this; d != root && d; d = d->parent() )
      path.prepend( d->name() );

   return path;
}

QString
File::humanReadableSize( UnitPrefix key /*= mega*/ ) const //FIXME inline
{
   return humanReadableSize( m_size, key );
}

QString
File::humanReadableSize( uint size, UnitPrefix key /*= mega*/ ) //static
{
   if( size == 0 )
      return "0 B";

   QString s;
   double prettySize = (double)size / (double)DENOMINATOR[key];
   const KLocale &locale = *KGlobal::locale();

   if( prettySize >= 0.01 )
   {
      //use three significant figures
      if( prettySize < 1 )        s = locale.formatNumber( prettySize, 2 );
      else if( prettySize < 100 ) s = locale.formatNumber( prettySize, 1 );
      else                        s = locale.formatNumber( prettySize, 0 );

      s += ' ';
      s += PREFIX[key];
      s += 'B';
   }

   if( prettySize < 0.1 )
   {
      s += " (";
      s += locale.formatNumber( size / DENOMINATOR[key - 1], 0 );
      s += ' ';
      s += PREFIX[key - 1];
      s += "B)";
   }

   return s;
}
