/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2014 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef HISTORYACTION_H
#define HISTORYACTION_H

#include <QAction>
#include <QUrl>

class KConfigGroup;
class KActionCollection;

class HistoryAction : QAction
{
    Q_OBJECT

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

public Q_SLOTS:
    void push(const QUrl& url);
    void stop() {
        m_receiver = nullptr;
    }

Q_SIGNALS:
    void activated(const QUrl&);

private Q_SLOTS:
    void pop();

private:
    HistoryAction *m_b, *m_f, *m_receiver;
};

#endif
