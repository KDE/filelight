/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2014  Martin Sandsmark <martin.sandsmark@kde.org>
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

#ifndef HISTORYACTION_H
#define HISTORYACTION_H

#include <QAction>
#include <QStringList>
#include <QUrl>

class KConfigGroup;
class KActionCollection;

class HistoryAction : QAction
{
    HistoryAction(const QIcon &icon, const QString &text, KActionCollection *ac);

    friend class HistoryCollection;

public:
    virtual void setEnabled(bool b = true) {
        QAction::setEnabled(b && !m_list.isEmpty());
    }

    void clear() {
        m_list.clear();
        setEnabled(false);
        QAction::setText(m_text);
    }

private:
    void setHelpText(const QUrl& url);

    void push(const QUrl &url);
    QUrl pop();

    const QString m_text;
    QList<QUrl> m_list;
};


class HistoryCollection : public QObject
{
    Q_OBJECT

public:
    HistoryCollection(KActionCollection *ac, QObject *parent);

    void save(KConfigGroup &configgroup);
    void restore(const KConfigGroup &configgroup);

public slots:
    void push(const QUrl& url);
    void stop() {
        m_receiver = 0;
    }

signals:
    void activated(const QUrl&);

private slots:
    void pop();

private:
    HistoryAction *m_b, *m_f, *m_receiver;
};

#endif
