/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "historyAction.h"
#include "filelight_debug.h"
#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardShortcut>
#include <KLocalizedString>


#include <QIcon>

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
        , m_b(new HistoryAction(QIcon::fromTheme(QStringLiteral( "go-previous" )), i18nc("Go to the last path viewed", "Back"), ac))
        , m_f(new HistoryAction(QIcon::fromTheme(QStringLiteral( "go-next" )), i18nc("Go to forward in the history of paths viewed", "Forward"), ac))
        , m_receiver(nullptr)
{
    ac->addAction(QStringLiteral( "go_back" ), m_b);
    ac->addAction(QStringLiteral( "go_forward" ), m_f);
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
    m_receiver = nullptr;
}

void HistoryCollection::pop() //slot
{
    QUrl url = ((HistoryAction*)sender())->pop();

    m_receiver = (sender() == m_b) ? m_f : m_b;

    Q_EMIT activated(url);
}

void HistoryCollection::save(KConfigGroup &configgroup)
{
    configgroup.writePathEntry("backHistory", QUrl::toStringList(m_b->m_list));
    configgroup.writePathEntry("forwardHistory", QUrl::toStringList(m_f->m_list));
}

void HistoryCollection::restore(const KConfigGroup &configgroup)
{
    if (!m_b || !m_f) {
        qCWarning(FILELIGHT_LOG) << "what the actual fuck";
        return;
    }

    m_b->m_list = QUrl::fromStringList(configgroup.readPathEntry("backHistory", QStringList()));
    m_f->m_list = QUrl::fromStringList(configgroup.readPathEntry("forwardHistory", QStringList()));
    //TODO texts are not updated - no matter
}


