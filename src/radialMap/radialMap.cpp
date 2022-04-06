/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "radialMap.h"

#include "fileTree.h"

RadialMap::Segment::~Segment()
{
    if (isFake()) {
        delete m_file; // created by us in Builder::build()
    }
}
