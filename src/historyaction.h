/***************************************************************************
                          historyaction.h  -  description
                             -------------------
    begin                : Tue Sep 16 2003
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

#ifndef HISTORYACTION_H
#define HISTORYACTION_H

#include <kaction.h>



class QString;
class QStringList;

class HistoryAction : public KAction
{
  Q_OBJECT

  public:

    HistoryAction( const QString &s, const QString& pix, const KShortcut &cut, const QObject *receiver, const char *slot, KActionCollection *parent, const char *name )
     : KAction( s, pix, cut, receiver, slot, parent, name ), m_prefix( s )
       { setEnabled( false ); }

    void push( const QString &s )
    {
      if( s != QString::null && s != m_history.last() ) {
        setText( s );
        m_history.append( s );
        if( !isEnabled() ) setEnabled( true );
      }
    }

    const QString pop() //will crash if empty mind
    {
      const QString s = m_history.last();

      m_history.pop_back();

      if( !m_history.isEmpty() )
        setText( m_history.last() );
      else {
        clearText();
        setEnabled( false );
      }

      return s;
    }

    void setHistory( const QStringList &list ) { m_history = list; setEnabled( true ); setText( m_history.last() ); } //friend this to saveProperties at some point
    void clear() { if( !m_history.isEmpty() ) { m_history.clear(); clearText(); setEnabled( false ); } }

    const QStringList &history() const { return m_history; }

  public slots:
    virtual void setEnabled( bool b ) { if( b && m_history.isEmpty() ) b = false; KAction::setEnabled( b ); }

  private:
    void clearText() { KAction::setText( m_prefix ); }
    //**** internationalise this bit ->
    void setText( const QString &s ) { KAction::setText( m_prefix + ": " + s ); }

    QStringList m_history;
    QString     m_prefix;
};

#endif


/*

#include <qobject.h>

class QStringList;
class KMainWindow;
class KAction;



#include <qstringlist.h>
#include <kmainwindow.h>
#include <kstdaction.h>


//deleting this yourself will give undefined behaviour!

class Histories : public QObject
{
Q_OBJECT

public:
    Histories( KMainWindow *parent, const char *member, const char *name = 0 ) : QObject( parent, name )
    {
      connect( this, SIGNAL( activated() ), parent, SLOT( member ) ) );

      m_back = KStdAction::back( this, SLOT( back ), parent->actionCollection() );
      m_fwd  = KStdAction::forward( this, SLOT( forward ), parent->actionCollection() );

      m_status = Histories::None;
    }

public slots:
    void historify( const QString &historicPath )
    {
      //check for empty/Null etc. valid? make that a function to pass by addr

      switch( m_status ) {
      case Histories::None:
        m_fwdHist.clear();
      case Histories::Forward:
        m_fwdHist.pop_back();
        m_back.append( historicPath );
        m_back.setText( QString( "Back: " ) + historicPath );
        break;
      case Histories::Back:
        m_backHist.pop_back();
        m_fwdHist.append( historicPath );
        m_fwdHist.setText( QString( "Forward: " ) + historicPath );
      default:
        break;

      m_status = Histories::None;
    }

    void cancel() { m_status = Histories::None; }
            
private slots:
    void back() { emit activated( m_backHist.last(); m_status = Histories::Back );
    void forward() { emit activated( m_fwdHist.last(); m_status = Histories::Forward );

signals:
    activated( const QString & );

private:
    enum Status { None, Back, Forward }

    QStringList m_backHist, m_fwdHist;
    KAction    *m_back, m_fwd;
    Status      m_status;
};

*/
