//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

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
        Builder( Map*, const Directory* const, bool fast=false );

    private:
        void findVisibleDepth( const Directory* const dir, const uint=0 );
        void setLimits( const uint& );
        bool build( const Directory* const, const uint=0, uint=0, const uint=5760 );

        Map             *m_map;
        const Directory* const m_root;
        const uint       m_minSize;
        uint            *m_depth;
        Chain<Segment>  *m_signature;
        uint            *m_limits;
    };
}

#endif
