/***************************************************************************
 *   Copyright 2003-6 Max Howell <max.howell@methylblue.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CRASH_H
#define CRASH_H

#include <kcrash.h> //for main.cpp


/**
 * @author Max Howell
 * @short The Amarok crash-handler
 *
 * I'm not entirely sure why this had to be inside a class, but it
 * wouldn't work otherwise *shrug*
 */
struct mxcl
{
    static void crashHandler( int signal );

private:
    mxcl();
    mxcl( mxcl& );
    mxcl &operator=( mxcl& );
    bool operator==( mxcl& );
};

#endif
