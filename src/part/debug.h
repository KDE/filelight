/****************************************************************************
**
** Copyright (C) 1992-2005 Trolltech AS. All rights reserved.
** Copyright (C) 2006 Max Howell <max.howell@methylblue.com>
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/// Quite a lot modified by Max Howell so it works with Qt3


#ifndef QDEBUG_H
#define QDEBUG_H

#include <qstringlist.h>


struct ForEachHelper
{
   QStringList::ConstIterator const end;
   QStringList::ConstIterator it;

   bool m_break;

   ForEachHelper( QStringList const &list )
         : end( list.end() )
         , it( list.begin() )
         , m_break( true )
   {}
};

#define foreach( var, list ) \
   for (ForEachHelper h( list ); h.it != h.end; ++h.it, h.m_break = true) \
      for (var = *h.it; h.m_break; h.m_break = false)


#ifdef QT_NO_DEBUG
   #define NDEBUG
#endif

#ifndef NDEBUG

#include <qcstring.h>
#include <qtextstream.h>
#include <kdebug.h>
#include <kurl.h>


class QDebug
{
    friend class QDebugBlock;

    static QString indent;

    struct Stream {
        Stream(QtMsgType t) : ts(&buffer, IO_WriteOnly), ref(1), type(t), space(true){}
        QTextStream ts;
        QString buffer;
        int ref;
        QtMsgType type;
        bool space;
    } *stream;

public:
    inline QDebug( QtMsgType t ) : stream( new Stream( t ) ) {}
    inline QDebug( const QDebug &o ) : stream( o.stream ) { ++stream->ref; }
    inline ~QDebug()
    {
        if (!--stream->ref) {
            stream->ts << "\n";
            switch (stream->type) {
               default:
               case QtDebugMsg: kdDebug() << indent << stream->buffer; break;
               case QtWarningMsg: kdWarning() << indent << stream->buffer; break;
            }
            delete stream;
        }
    }

    inline QDebug &space() { stream->space = true; stream->ts << " "; return *this; }
    inline QDebug &nospace() { stream->space = false; return *this; }
    inline QDebug &maybeSpace() { if (stream->space) stream->ts << " "; return *this; }

    inline QDebug &operator<<(QChar t) { stream->ts << "\'" << t << "\'"; return maybeSpace(); }
    inline QDebug &operator<<(bool t) { stream->ts << (t ? "true" : "false"); return maybeSpace(); }
    inline QDebug &operator<<(char t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(signed short t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(unsigned short t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(signed int t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(unsigned int t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(signed long t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(unsigned long t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(float t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(double t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(const char* t) { stream->ts  << t; return maybeSpace(); }
    inline QDebug &operator<<(const QString & t) { stream->ts << "\"" << t  << "\""; return maybeSpace(); }

    inline QDebug &operator<<(const QStringList &list)
    {
        stream->ts << "(";
        foreach (QString s, list)
            stream->ts << s << ", ";
        stream->ts << ")";
        return maybeSpace();
    }

    //NOTE added by mxcl
    inline QDebug &operator<<(const KURL &url) { stream->ts << '"' << url.prettyURL() << '"'; return maybeSpace(); }
    inline QDebug &operator<<(const QCString &s) { stream->ts << '"' << s << '"'; return maybeSpace(); }
};

inline QDebug qDebug() { return QDebug( QtDebugMsg ); }
inline QDebug qWarning() { return QDebug( QtWarningMsg ); }
inline QDebug qError() { return QDebug( QtWarningMsg ); }

class QDebugBlock
{
	const char *m_title;

public:
	QDebugBlock( const char *title )
		: m_title( title )
	{
		qDebug() << "BEGIN:" << title;
		QDebug::indent += "  ";
	}

	~QDebugBlock()
	{
		QDebug::indent.truncate( QDebug::indent.length() - 2 );
		qDebug() << "END__:" << m_title;
	}
};

#define Q_DEBUG_BLOCK QDebugBlock q_debug_block( __PRETTY_FUNCTION__ );

#else // NDEBUG

class QNoDebug
{
public:
    inline QNoDebug() {}
    inline QNoDebug( const QNoDebug& ) {}
    inline ~QNoDebug() {}

    inline QNoDebug &space() { return *this; }
    inline QNoDebug &nospace() { return *this; }
    inline QNoDebug &maybeSpace() { return *this; }

    template<typename T> inline QNoDebug &operator<<( const T& ) { return *this; }
};

inline QNoDebug qDebug() { return QNoDebug(); }

#define Q_DEBUG_BLOCK

#endif

#define DEBUG_ANNOUNCE qDebug() << ">>" << __PRETTY_FUNCTION__;

#endif // QDEBUG_H
