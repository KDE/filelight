/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef Config_H
#define Config_H

#include <QStringList>
#include <QSet>

class KConfig;

enum class Dirty
{
    Layout = 1,
    AntiAliasing = 2,
    Colors = 3,
    Font
};

namespace Filelight
{
enum MapScheme { Rainbow, KDE, HighContrast, FileDensity, ModTime };

class Config
{
public:
    static void read();
    static void write();

    //keep everything positive, avoid using DON'T, NOT or NO

    static bool scanAcrossMounts;
    static bool scanRemoteMounts;
    static bool varyLabelFontSizes;
    static bool showSmallFiles;
    static uint contrast;
    static bool antialias;
    static int minFontPitch;
    static uint defaultRingDepth;

    static MapScheme scheme;
    static QStringList skipList;

    static const QSet<QByteArray> remoteFsTypes;
};
}

using Filelight::Config;

#endif
