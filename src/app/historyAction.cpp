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

#include "historyAction.h"

#include <KAction>
#include <KConfig>
#include <KLocale>
#include <KActionCollection>
#include <KStandardShortcut>
#include <KConfigGroup>


inline HistoryAction::HistoryAction(const QString &text, KActionCollection *ac)
        : KAction(text, ac)
        , m_text(text)
{
    // .ui files make this false, but we can't rely on UI file as it isn't compiled in :(
    KAction::setEnabled(false);
}

void HistoryAction::push(const QString &path)
{
    if (!path.isEmpty() && !m_list.isEmpty() && m_list.last() != path)
    {
        m_list.append(path);
        setActionMenuTextOnly(this, path);
        KAction::setEnabled(true);
    }
}

QString HistoryAction::pop()
{
    const QString s = m_list.last();
    m_list.pop_back();
    setActionMenuTextOnly(this, m_list.last());
    setEnabled();
    return s;
}



HistoryCollection::HistoryCollection(KActionCollection *ac, QObject *parent)
        : QObject(parent)
        , m_b(new HistoryAction(i18n("Back"), ac))
        , m_f(new HistoryAction(i18n("Forward"), ac))
        , m_receiver(0)
{
    connect(m_b, SIGNAL(activated()), SLOT(pop()));
    connect(m_f, SIGNAL(activated()), SLOT(pop()));
}

void HistoryCollection::push(const KUrl &url) //slot
{
    if (!url.isEmpty())
    {
        if (!m_receiver)
        {
            m_f->clear();
            m_receiver = m_b;
        }

        m_receiver->push(url.path(KUrl::AddTrailingSlash));
    }
    m_receiver = 0;
}

void HistoryCollection::pop() //slot
{
    KUrl url;
    const QString path = ((HistoryAction*)sender())->pop(); //FIXME here we remove the constness
    url.setPath(path);

    m_receiver = (sender() == m_b) ? m_f : m_b;

    emit activated(url);
}

void HistoryCollection::save(KConfigGroup &configgroup)
{
    configgroup.writePathEntry("backHistory", m_b->m_list);
    configgroup.writePathEntry("forwardHistory", m_f->m_list);
}

void HistoryCollection::restore(const KConfigGroup &configgroup)
{
    m_b->m_list = configgroup.readPathEntry("backHistory", QStringList());
    m_f->m_list = configgroup.readPathEntry("forwardHistory", QStringList());
    //TODO texts are not updated - no matter
}

#include "historyAction.moc"
