/***************************************************************************
                          historyaction.cpp  -  description
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "historyaction.h"


HistoryAction::HistoryAction( const QString &text, const char *icon, const KShortcut &cut, KActionCollection *ac, const char *name ) :
    KAction( text, icon, cut, 0, 0, ac, name ),
    m_text( text )
  {
    //**** wouldn't find slot it in KAction ctor for some reason
    connect( this, SIGNAL( activated() ), SLOT( pop() ) );
  }


HistoryCollection::HistoryCollection( KActionCollection *ac, QObject *parent, const char *name ) :
      QObject( parent, name ),
      m_b( new HistoryAction( i18n( "Back" ), "back", KStdAccel::back(), ac, "go_back" ) ),
      m_f( new HistoryAction( i18n( "Forward" ), "forward",  KStdAccel::forward(), ac, "go_forward" ) ),
      m_receiver( 0 )
    {
      connect( m_b, SIGNAL( activated( HistoryAction *, const QString & ) ), this, SLOT( process( HistoryAction *, const QString & ) ) );
      connect( m_f, SIGNAL( activated( HistoryAction *, const QString & ) ), this, SLOT( process( HistoryAction *, const QString & ) ) );
    }

void HistoryCollection::save( KConfig *config )
    {
      #if KDE_VERSION >= 0x030103
      config->writePathEntry( "backHistory", m_b->m_list );
      config->writePathEntry( "forwardHistory", m_f->m_list );
      #endif
    }

void HistoryCollection::restore( KConfig *config )
    {
      #if KDE_VERSION >= 0x030103
      m_b->m_list = config->readPathListEntry( "backHistory" );
      m_f->m_list = config->readPathListEntry( "forwardHistory" );
      //**** texts are not updated, no matter
      #endif
    }

#include "historyaction.moc"
