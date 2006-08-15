//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#ifndef HISTORYACTION_H
#define HISTORYACTION_H

#include <kaction.h>
#include <kurl.h>
#include <qstringlist.h>

class KConfig;


/// defined in mainWindow.cpp
void setActionMenuTextOnly( KAction *a, QString const &suffix );


class HistoryAction : KAction
{
    HistoryAction( const QString &text, const char *icon, const KShortcut &cut, KActionCollection *ac, const char *name );

    friend class HistoryCollection;

public:
    virtual void setEnabled( bool b = true ) { KAction::setEnabled( b ? !m_list.isEmpty() : false ); }

    void clear() { m_list.clear(); KAction::setText( m_text ); }

private:
    void setText();

    void push( const QString &path );
    QString pop();

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
