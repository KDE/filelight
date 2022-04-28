/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2017-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <QUrl>

#include <KXmlGuiWindow>

#include "Config.h"

class QLabel;
class QQmlApplicationEngine;

namespace RadialMap
{
class Item;
} // namespace RadialMap
class Folder;
class HistoryCollection;

namespace Filelight
{

class ScanManager;

class MainContext : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_SIGNAL void urlChanged();
    void setUrl(const QUrl &url);
    QUrl m_url;

    Q_PROPERTY(QList<QObject *> historyActions MEMBER m_historyActions NOTIFY historyActionsChanged)
    Q_SIGNAL void historyActionsChanged();
    QList<QObject *> m_historyActions;

    explicit MainContext(QObject *parent = nullptr);

    void addHistoryAction(QObject *action);
    Q_SLOT void scan(const QUrl &u);

Q_SIGNALS:
    void canceled(const QString &);
    void canvasIsDirty(Dirty filth);

public Q_SLOTS:
    void slotUp();
    void slotScanFolder();
    void slotScanHomeFolder();
    void slotScanRootFolder();
    bool slotScanUrl(const QUrl &);
    bool slotScanPath(const QString &);

    void urlAboutToChange();

    bool openUrl(const QUrl &);
    void configFilelight();

    void updateURL(const QUrl &);
    void rescanSingleDir(const QUrl &);

private:
    void setupActions(QQmlApplicationEngine *engine);

public:
    Q_INVOKABLE QString prettyUrl(const QUrl &url) const;
    Q_INVOKABLE bool start(const QUrl &) const;

    HistoryCollection *m_histories;
    ScanManager *m_manager;

public:
    QUrl url() const;
};

} // namespace Filelight
