// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

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
