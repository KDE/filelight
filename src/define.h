/***************************************************************************
                          define.h  -  description
                             -------------------
    begin                : Tue Sep 23 2003
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

#ifndef DEFINE_H
#define DEFINE_H

#ifndef PI
#define PI 3.141592653589793
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#define LABEL_MAP_SPACER 7    //distance from map to horizontal strut of labels
#define LABEL_HMARGIN 10
#define LABEL_TEXT_HMARGIN 5
#define LABEL_TEXT_VMARGIN 0
#define LABEL_ANGLE_MARGIN 32 //in 16ths of degree
#define LABEL_MIN_ANGLE_FACTOR 0.05
#define LABEL_MAX_CHARS 30

#define PRETTYNAME "Filelight"

#define MIN_RING_BREADTH 20
#define MAX_RING_BREADTH 60
#define DEFAULT_RING_DEPTH 4 //first level = 0
#define MIN_RING_DEPTH 0


//factor for filesizes in scan, i.e. show in kB, MB, GB or TB (TB only possible on 64bit systems)
const unsigned int UNIT_DENOMINATOR[4] = { 1, 1024, 1048576, 1073741824 };
const char UNIT_PREFIX[4] = { 'k', 'M', 'G', 'T' };

enum UnitPrefix { kilo, mega, giga, tera };

class File;
class Directory;

QString fullPath( const File *, const Directory * const = 0 );
QString makeHumanReadable( unsigned int, UnitPrefix = mega );

#endif
