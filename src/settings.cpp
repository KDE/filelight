/***************************************************************************
                          settings.cpp  -  description
                             -------------------
    begin                : Mon Sep 1 2003
    copyright            : (C) 2003 by Max Howell
    email                : max.howell@methylblue.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kconfig.h>
#include <qstringlist.h>
#include <qfont.h>

#include "settings.h"



bool Settings::readSettings()
{
  //**** need to return false somehow if error, but how detect error?
  KConfig config( m_path );

  config.setGroup( "filelight_part" );
  scanAcrossMounts   = config.readBoolEntry( "scanAcrossMounts", false );
  scanRemoteMounts   = config.readBoolEntry( "scanRemoteMounts", false );
  scanRemovableMedia = config.readBoolEntry( "scanRemovableMedia", false );
  skipList           = config.readPathListEntry( "skipList" );
  scheme             = (MapScheme)config.readNumEntry( "scheme", 0 );
  contrast           = config.readNumEntry( "contrast", 50 );
  aaFactor           = config.readNumEntry( "aaFactor", 2 );
  varyLabelFontSizes = config.readBoolEntry( "varyLabelFontSizes", true );
  minFontPitch       = config.readNumEntry( "minFontPitch", QFont().pointSize() - 3);
  showSmallFiles     = config.readBoolEntry( "showSmallFiles", false );

  defaultRingDepth = 4;

  return true;
}


bool Settings::writeSettings()
{
  KConfig config( m_path );

  config.setGroup( "filelight_part" );

  config.writeEntry( "scanAcrossMounts", scanAcrossMounts );
  config.writeEntry( "scanRemoteMounts", scanRemoteMounts );
  config.writeEntry( "scanRemovableMedia", scanRemovableMedia );
  config.writePathEntry( "skipList", skipList );

  config.writeEntry( "scheme", scheme );
  config.writeEntry( "contrast", contrast );
  config.writeEntry( "aaFactor", aaFactor );
  config.writeEntry( "varyLabelFontSizes", varyLabelFontSizes );
  config.writeEntry( "minFontPitch", minFontPitch );
  config.writeEntry( "showSmallFiles", showSmallFiles);

  return true;
}
