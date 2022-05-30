/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <math.h>

#ifndef HAVE_SINCOS

#include <qmath.h>

static inline void sincos(double angleRadians, double *Sin, double *Cos)
{
    *Sin = qSin(angleRadians);
    *Cos = qCos(angleRadians);
}

#endif
