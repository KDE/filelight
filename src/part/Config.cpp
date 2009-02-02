/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <sandsmark@samfundet.no>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/


#include "Config.h"

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KSharedConfig>
#include <KSharedPtr>


bool Config::scanAcrossMounts;
bool Config::scanRemoteMounts;
bool Config::scanRemovableMedia;
bool Config::varyLabelFontSizes;
bool Config::showSmallFiles;
uint Config::contrast;
uint Config::antiAliasFactor;
uint Config::minFontPitch;
uint Config::defaultRingDepth;
Filelight::MapScheme Config::scheme;
QStringList Config::skipList;

void
Filelight::Config::read()
{
    const KConfigGroup config = KGlobal::config()->group("filelight_part");

    scanAcrossMounts   = config.readEntry("scanAcrossMounts", false);
    scanRemoteMounts   = config.readEntry("scanRemoteMounts", false);
    scanRemovableMedia = config.readEntry("scanRemovableMedia", false);
    varyLabelFontSizes = config.readEntry("varyLabelFontSizes", true);
    showSmallFiles     = config.readEntry("showSmallFiles", false);
    contrast           = config.readEntry("contrast", 75);
    antiAliasFactor    = config.readEntry("antiAliasFactor", 2);
    minFontPitch       = config.readEntry("minFontPitch", QFont().pointSize() - 3);
    scheme = (MapScheme) config.readEntry("scheme", 0);
    skipList           = config.readEntry("skipList", QStringList());

    defaultRingDepth   = 4;
}

void
Filelight::Config::write()
{
    KConfigGroup config = KGlobal::config()->group("filelight_part");

    config.writeEntry("scanAcrossMounts", scanAcrossMounts);
    config.writeEntry("scanRemoteMounts", scanRemoteMounts);
    config.writeEntry("scanRemovableMedia", scanRemovableMedia);
    config.writeEntry("varyLabelFontSizes", varyLabelFontSizes);
    config.writeEntry("showSmallFiles", showSmallFiles);
    config.writeEntry("contrast", contrast);
    config.writeEntry("antiAliasFactor", antiAliasFactor);
    config.writeEntry("minFontPitch", minFontPitch);
    config.writeEntry("scheme", (int)scheme); // TODO: make the enum belong to a qwidget,
    //and use magic macros to make it save this properly
    config.writePathEntry("skipList", skipList);
}
