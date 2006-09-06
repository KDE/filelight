// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#ifndef SEGMENT_TIP_H
#define SEGMENT_TIP_H

#include <kpixmap.h>
#include <qwidget.h>

class File;
class Directory;

namespace RadialMap
{
    class SegmentTip : public QWidget
    {
    public:
        explicit SegmentTip( uint );

        void updateTip( const File*, const Directory* );
        void moveTo( QPoint, const QWidget&, bool );

    private:
        virtual bool eventFilter( QObject*, QEvent* );
        virtual bool event( QEvent* );

        uint    m_cursorHeight;
        KPixmap m_pixmap;
        QString m_text;
        bool    m_backing_store;
    };
}

#endif
