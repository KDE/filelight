//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#ifndef HISTORYACTION_H
#define HISTORYACTION_H

#include <kaction.h>
#include <kurl.h>
#include <qstringlist.h>

class KConfig;


class HistoryAction : KAction
{
    HistoryAction( const QString &text, const char *icon, const KShortcut &cut, KActionCollection *ac, const char *name );

    friend class HistoryCollection;

public:
    virtual void setEnabled( bool b = true ) { KAction::setEnabled( b ? !m_list.isEmpty() : false ); }

    void clear() { m_list.clear(); KAction::setText( m_text ); }

private:
    void setText()
    {
        QString newText = m_text;
        if( !m_list.isEmpty() ) { newText += ": "; newText += m_list.last(); }
        KAction::setText( newText );
    }
    void push( const QString &path )
    {
        if( !path.isEmpty() && m_list.last() != path )
        {
            m_list.append( path );
            setText();
            KAction::setEnabled( true );
        }
    }
    QString pop()
    {
        const QString s = m_list.last();
        m_list.pop_back();
        setText();
        setEnabled();
        return s;
    }

    const QString m_text;
    QStringList m_list;
};


class HistoryCollection : public QObject
{
Q_OBJECT

public:
    HistoryCollection( KActionCollection *ac, QObject *parent, const char *name );

    void save( KConfig *config );
    void restore( KConfig *config );

public slots:
    void push( const KURL& );
    void stop() { m_receiver = 0; }

signals:
    void activated( const KURL& );

private slots:
    void pop();

private:
    HistoryAction *m_b, *m_f, *m_receiver;
};

#endif
