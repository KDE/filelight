//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#ifndef SINCOS_H
#define SINCOS_H

#include <math.h>

#if !defined(__GLIBC__) || (__GLIBC__ < 2) ||  (__GLIBC__ == 2 && __GLIBC_MINOR__ < 1)

   void
   sincos( int angleRadians, double *Sin, double *Cos );

#ifdef SINCOS_H_IMPLEMENTATION
   void
   sincos( int angleRadians, double *Sin, double *Cos )
   {
      *Sin = sin( angleRadians );
      *Cos = cos( angleRadians );
   }
#endif

#endif

#endif
