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

#ifndef BUILDER_H
#define BUILDER_H

#include "radialMap.h"   //Segment, defines

template <class T> class Chain;
class Directory;


namespace RadialMap
{
class Map;

//temporary class that builds the Map signature

class Builder
{
public:
    Builder(Map*, const Directory* const, bool fast=false);

private:
    void findVisibleDepth(const Directory* const dir, const uint=0);
    void setLimits(const uint&);
    bool build(const Directory* const, const uint=0, uint=0, const uint=5760);

    Map             *m_map;
    const Directory* const m_root;
    const uint       m_minSize;
    uint            *m_depth;
    Chain<Segment>  *m_signature;
    uint            *m_limits;
};
}

#endif
