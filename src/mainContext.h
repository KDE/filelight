/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2017-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <QUrl>

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

    Q_PROPERTY(QList<QObject *> historyActions MEMBER m_historyActions NOTIFY historyActionsChanged)

    explicit MainContext(QObject *parent = nullptr);
    [[nodiscard]] QUrl url() const;

Q_SIGNALS:
    void canceled(const QString &);
    void canvasIsDirty(Filelight::Dirty filth);
    void urlChanged();
    void historyActionsChanged();
    void openUrlFailed(const QString &text, const QString &explanation);

public Q_SLOTS:
    void scan(const QUrl &u);

    void slotUp();
    void slotScanFolder();
    void slotScanHomeFolder();
    void slotScanRootFolder();
    bool slotScanUrl(const QUrl &);
    bool slotScanPath(const QString &);

    bool openUrl(const QUrl &);

    void updateURL(const QUrl &);
    void rescanSingleDir(const QUrl &) const;

private:
    void setupActions(QQmlApplicationEngine *engine);

    void addHistoryAction(QObject *action);

    /// For internal use only -- call openUrl() instead
    void setUrl(const QUrl &url);

    QUrl m_url;
    HistoryCollection *m_histories;
    ScanManager *m_manager;
    QList<QObject *> m_historyActions;

public:
    Q_INVOKABLE QString prettyUrl(const QUrl &url) const;
    Q_INVOKABLE bool start(const QUrl &) const;
};

} // namespace Filelight
