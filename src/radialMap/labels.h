/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QVector>

#include "Config.h"
#include "fileTree.h"
#include "radialMap.h"
#include "sincos.h"

namespace RadialMap
{
class Label
{
public:
    Label(const RadialMap::Segment *s, int l)
        : segment(s)
        , level(l)
        , angle(segment->start() + (segment->length() / 2))
    {
    }

    bool tooClose(const int otherAngle) const
    {
        return (angle > otherAngle - LABEL_ANGLE_MARGIN && angle < otherAngle + LABEL_ANGLE_MARGIN);
    }

    const RadialMap::Segment *segment;
    const unsigned int level;
    const int angle;

    int targetX, targetY, middleX, startY, startX;
    int textX, textY, tw, th;

    QString qs;
};

} // namespace RadialMap
