// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#ifndef DEBUG_H
#define DEBUG_H

/** Fancy debug header
 * @author Max Howell
 *
 * Define DEBUG_PREFIX as a string before you include this to insert a fancy debug prefix
 * Debug::debug(), can be used as debug() and is just kdDebug()
 * use Debug::indent() and Debug::unindent()
 */

#include <kdebug.h>

#ifdef NDEBUG
static inline kndbgstream debug() { return kndbgstream(); }
#else
static inline kdbgstream debug()
{
   return kdbgstream(
   #ifdef DEBUG_PREFIX
      "[" DEBUG_PREFIX "] ",
   #endif
      0, 0 );
}
#endif

#define error   kdError
#define fatal   kdFatal
#define warning kdWarning

#define DEBUG_ANNOUNCE debug() << ">> " << __PRETTY_FUNCTION__ << endl;

#endif
