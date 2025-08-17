/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2017-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "mainContext.h"

#include <KAboutData>
#include <KIO/Global> // upUrl
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QBindable>
#include <QDir>
#include <QFileDialog>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>

#include "contextMenuContext.h"
#include "fileModel.h"
#include "radialMap/map.h"
#include "radialMap/radialMap.h"
#include "scan.h"

using namespace Qt::StringLiterals;

namespace Filelight
{

MainContext::MainContext(QObject *parent)
    : QObject(parent)
    , m_manager(ScanManager::instance())
{
    qRegisterMetaType<size_t>("size_t");

    auto fileModel = FileModel::instance();

    connect(m_manager, &ScanManager::completed, RadialMap::Map::instance(), [](const auto &tree) {
        if (tree) {
            RadialMap::Map::instance()->make(tree);
        }
    });

    connect(m_manager, &ScanManager::aborted, fileModel, [fileModel]() {
        fileModel->setTree({});
    });

    connect(RadialMap::Map::instance(), &RadialMap::Map::signatureChanged, fileModel, [fileModel]() {
        const auto tree = RadialMap::Map::instance()->root();
        fileModel->setTree(tree);
    });
}

void MainContext::scan(const QString &u)
{
    auto url = QUrl::fromUserInput(u, QDir::currentPath(), QUrl::AssumeLocalFile);
    slotScanUrl(url);
}

void MainContext::slotScanFolder()
{
    slotScanUrl(QFileDialog::getExistingDirectoryUrl(nullptr, i18n("Select Folder to Scan"), url()));
}

void MainContext::slotScanHomeFolder()
{
    slotScanPath(QDir::homePath());
}

void MainContext::slotScanRootFolder()
{
    slotScanPath(QDir::rootPath());
}

void MainContext::slotUp()
{
    const auto downUrl = url();
    auto upUrl = KIO::upUrl(downUrl);
#ifdef Q_OS_WINDOWS
    if (upUrl.path() == QLatin1Char('/')) { // root means nothing on windows
        upUrl = downUrl;
    }
#endif
    slotScanUrl(upUrl);
}

bool MainContext::slotScanPath(const QString &path)
{
    return slotScanUrl(QUrl::fromUserInput(path));
}

bool MainContext::slotScanUrl(const QUrl &url)
{
    const QUrl oldUrl = this->url();

    if (openUrl(url)) {
        return true;
    }
    return false;
}

bool MainContext::openUrl(const QUrl &u)
{
    // TODO should have an empty QUrl until scan is confirmed successful
    // TODO probably should set caption to QString::null while map is unusable

    QUrl uri = u.adjusted(QUrl::NormalizePathSegments);
    const QString localPath = uri.toLocalFile();
    const bool isLocal = uri.isLocalFile();

    if (uri.isEmpty()) {
        // do nothing, chances are the user accidentally pressed ENTER
    } else if (!uri.isValid()) {
        Q_EMIT openUrlFailed(i18n("The entered URL cannot be parsed"), i18n("it is invalid."));
    } else if (isLocal && !QDir::isAbsolutePath(localPath)) {
        Q_EMIT openUrlFailed(i18n("Filelight only accepts absolute paths"), i18n("eg. /%1", localPath));
    } else if (isLocal && !QDir(localPath).exists()) {
        Q_EMIT openUrlFailed(i18n("Folder not found: %1", localPath), QString());
    } else if (isLocal && !QDir(localPath).isReadable()) {
        Q_EMIT openUrlFailed(i18n("Unable to enter: %1", localPath), i18n("You do not have access rights to this location."));
    } else {
        const bool success = start(uri);
        if (success) {
            setUrl(uri);
            setUpEnabled(url() != KIO::upUrl(url()));
        }
        return success;
    }

    qDebug() << "failed to openurl" << u;
    return false;
}

QString MainContext::prettyUrl(const QUrl &url) const
{
    return url.isLocalFile() ? QDir::toNativeSeparators(url.toLocalFile()) : url.toString();
}

void MainContext::updateURL(const QUrl &u)
{
    if (m_manager->running()) {
        m_manager->abort();
    }

    if (u == url()) {
        m_manager->emptyCache(); // same as rescan()
    }

    // do this last, or it breaks Konqi location bar
    setUrl(u);
}

void MainContext::rescanSingleDir(const QUrl &dirUrl) const
{
    if (m_manager->running()) {
        m_manager->abort();
    }

    m_manager->invalidateCacheFor(dirUrl);
    start(url());
}

QUrl MainContext::url() const
{
    return m_url;
}

void MainContext::setUrl(const QUrl &url)
{
    if (m_url != url) {
        m_url = url;
        Q_EMIT urlChanged();
    }
}

void MainContext::setUpEnabled(const bool enabled)
{
    if (m_upEnabled != enabled) {
        m_upEnabled = enabled;
        Q_EMIT upEnabledChanged();
    }
}

bool MainContext::start(const QUrl &url) const
{
    if (m_manager->running()) {
        m_manager->abort();
    }
    return m_manager->start(url);
}

MainContext *MainContext::create([[maybe_unused]] QQmlEngine *qml, [[maybe_unused]] QJSEngine *js)
{
    return new MainContext;
}
} // namespace Filelight

#include "mainContext.moc"
