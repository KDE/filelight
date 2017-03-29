/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
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

#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardShortcut>
#include <KLocalizedString>


#include <QIcon>
#include <QAction>
#include <QDebug>

inline HistoryAction::HistoryAction(const QIcon& icon, const QString& text, KActionCollection* ac)
        : QAction(icon, text, ac)
        , m_text(text)
{
    // .ui files make this false, but we can't rely on UI file as it isn't compiled in :(
    setEnabled(false);
}

void HistoryAction::setHelpText(const QUrl& url)
{
    QString text = url.path();
    setStatusTip(text);
    setToolTip(text);
    setWhatsThis(text);
}


void HistoryAction::push(const QUrl &path)
{
    if (path.isEmpty()) return;

    if (m_list.isEmpty() || (!m_list.isEmpty() && (m_list.last() != path)))
        m_list.append(path);

    setHelpText(path);
    setEnabled(true);
}

QUrl HistoryAction::pop()
{
    const QUrl s = m_list.takeLast();
    if (!m_list.isEmpty())
        setHelpText(m_list.last());

    setEnabled();
    return s;
}



HistoryCollection::HistoryCollection(KActionCollection *ac, QObject *parent)
        : QObject(parent)
        , m_b(new HistoryAction(QIcon::fromTheme(QLatin1String( "go-previous" )), i18nc("Go to the last path viewed", "Back"), ac))
        , m_f(new HistoryAction(QIcon::fromTheme(QLatin1String( "go-next" )), i18nc("Go to forward in the history of paths viewed", "Forward"), ac))
        , m_receiver(0)
{
    ac->addAction(QLatin1String( "go_back" ), m_b);
    ac->addAction(QLatin1String( "go_forward" ), m_f);
    connect(m_b, &QAction::triggered, this, &HistoryCollection::pop);
    connect(m_f, &QAction::triggered, this, &HistoryCollection::pop);
}

void HistoryCollection::push(const QUrl& url) //slot
{
    if (!url.isEmpty())
    {
        if (!m_receiver)
        {
            m_f->clear();
            m_receiver = m_b;
        }

        m_receiver->push(url);
    }
    m_receiver = 0;
}

void HistoryCollection::pop() //slot
{
    QUrl url = ((HistoryAction*)sender())->pop();

    m_receiver = (sender() == m_b) ? m_f : m_b;

    emit activated(url);
}

void HistoryCollection::save(KConfigGroup &configgroup)
{
    configgroup.writePathEntry("backHistory", QUrl::toStringList(m_b->m_list));
    configgroup.writePathEntry("forwardHistory", QUrl::toStringList(m_f->m_list));
}

void HistoryCollection::restore(const KConfigGroup &configgroup)
{
    if (!m_b || !m_f) {
        qWarning() << "what the actual fuck";
        return;
    }

    m_b->m_list = QUrl::fromStringList(configgroup.readPathEntry("backHistory", QStringList()));
    m_f->m_list = QUrl::fromStringList(configgroup.readPathEntry("forwardHistory", QStringList()));
    //TODO texts are not updated - no matter
}


