/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef SINCOS_H
#define SINCOS_H

#include <math.h>

#if !defined(__GLIBC__) || (__GLIBC__ < 2) ||  (__GLIBC__ == 2 && __GLIBC_MINOR__ < 1)

#include <qmath.h>

void
sincos(double angleRadians, double *Sin, double *Cos);

#ifdef SINCOS_H_IMPLEMENTATION
void
sincos(double angleRadians, double *Sin, double *Cos)
{
    *Sin = qSin(angleRadians);
    *Cos = qCos(angleRadians);
}
#endif

#endif

#endif
