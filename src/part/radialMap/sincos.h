//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#ifndef SINCOS_H
#define SINCOS_H

#include <math.h>

#if __GLIBC__ < 2 ||  __GLIBC__ == 2 && __GLIBC_MINOR__ < 1

   void
   sincos( int angleRadians, int *Sin, int *Cos )
   {
      *Sin = sin( angleRadians );
      *Cos = cos( angleRadians );
   }

#endif

#endif
