//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include "historyAction.h"

#include <kaccel.h>
#include <kconfig.h>
#include <klocale.h>


inline
HistoryAction::HistoryAction( const QString &text, const char *icon, const KShortcut &cut, KActionCollection *ac, const char *name )
        : KAction( text, icon, cut, 0, 0, ac, name )
        , m_text( text )
{
    // ui files make this false, but we can't rely on UI file as it isn't compiled in :(
    KAction::setEnabled( false );
}

void
HistoryAction::push( const QString &path )
{
    if( !path.isEmpty() && m_list.last() != path )
    {
        m_list.append( path );
        setActionMenuTextOnly( this, path );
        KAction::setEnabled( true );
    }
}

QString
HistoryAction::pop()
{
    const QString s = m_list.last();
    m_list.pop_back();
    setActionMenuTextOnly( this, m_list.last() );
    setEnabled();
    return s;
}



HistoryCollection::HistoryCollection( KActionCollection *ac, QObject *parent, const char *name )
        : QObject( parent, name )
        , m_b( new HistoryAction( i18n( "Back" ), "back", KStdAccel::back(), ac, "go_back" ) )
        , m_f( new HistoryAction( i18n( "Forward" ), "forward",  KStdAccel::forward(), ac, "go_forward" ) )
        , m_receiver( 0 )
{
    connect( m_b, SIGNAL(activated()), SLOT(pop()) );
    connect( m_f, SIGNAL(activated()), SLOT(pop()) );
}

void
HistoryCollection::push( const KURL &url ) //slot
{
    if( !url.isEmpty() )
    {
        if( !m_receiver )
        {
            m_f->clear();
            m_receiver = m_b;
        }

        m_receiver->push( url.path( 1 ) );
    }
    m_receiver = 0;
}

void
HistoryCollection::pop() //slot
{
    KURL url;
    const QString path = ((HistoryAction*)sender())->pop(); //FIXME here we remove the constness
    url.setPath( path );

    m_receiver = (sender() == m_b) ? m_f : m_b;

    emit activated( url );
}

void
HistoryCollection::save( KConfig *config )
{
    config->writePathEntry( "backHistory", m_b->m_list );
    config->writePathEntry( "forwardHistory", m_f->m_list );
}

void
HistoryCollection::restore( KConfig *config )
{
    m_b->m_list = config->readPathListEntry( "backHistory" );
    m_f->m_list = config->readPathListEntry( "forwardHistory" );
    //TODO texts are not updated - no matter
}

#include "historyAction.moc"
