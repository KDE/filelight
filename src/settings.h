/***************************************************************************
                          settings.h  -  description
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

#ifndef SETTINGS_H
#define SETTINGS_H


enum MapScheme { scheme_rainbow, scheme_highContrast, scheme_kde, scheme_fileDensity };


class KConfig;
class QStringList;

struct Settings
{
  //keep everything positive, avoid using DON'T, NOT or NO
public:
  bool scanAcrossMounts;
  bool scanRemoteMounts;
  bool scanRemovableMedia;
  QStringList skipList;
  MapScheme scheme;
  int  contrast;
  int  aaFactor;
  bool varyLabelFontSizes;
  int  minFontPitch;
  bool showSmallFiles;
  
  void useKConfig( KConfig *kc ) { config = kc; }
  bool readSettings();
  bool writeSettings();

private:
  KConfig *config; //don't want just anyone messing with the config variable, no no!
};

#endif
