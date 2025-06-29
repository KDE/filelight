/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2017-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "mainContext.h"
#include "ki18n_version.h"
#if KI18N_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <KLocalizedQmlContext>
#endif
#include <KAboutData>
#include <KIO/Global> // upUrl
#include <KLocalizedString>

#include <QBindable>
#include <QDir>
#include <QFileDialog>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>

#include "contextMenuContext.h"
#include "define.h"
#include "dropperItem.h"
#include "fileModel.h"
#include "radialMap/map.h"
#include "radialMap/radialMap.h"
#include "scan.h"
#include "windowThemer.h"

using namespace Qt::StringLiterals;

namespace Filelight
{

MainContext::MainContext(QObject *parent)
    : QObject(parent)
    , m_manager(new ScanManager(this))
{
    Config::instance()->read();

    auto engine = new QQmlApplicationEngine(this);

#if KI18N_VERSION < QT_VERSION_CHECK(6, 8, 0)
    static auto l10nContext = new KLocalizedContext(engine);
#else
    static auto l10nContext = new KLocalizedQmlContext(engine);
#endif
    l10nContext->setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));
    engine->rootContext()->setContextObject(l10nContext);

    qmlRegisterUncreatableMetaObject(Filelight::staticMetaObject, "org.kde.filelight", 1, 0, "Filelight", QStringLiteral("Access to enums & flags only"));

    qRegisterMetaType<size_t>("size_t");
    qmlRegisterType<DropperItem>("org.kde.filelight", 1, 0, "DropperItem");
    qmlRegisterType<WindowThemer>("org.kde.filelight", 1, 0, "WindowThemer");
    qmlRegisterSingletonInstance("org.kde.filelight", 1, 0, "ScanManager", m_manager);
    qmlRegisterSingletonInstance("org.kde.filelight", 1, 0, "MainContext", this);
    auto fileModel = new FileModel(this);
    qmlRegisterSingletonInstance("org.kde.filelight", 1, 0, "FileModel", fileModel);

    auto contextMenuContext = new ContextMenuContext(this);
    qmlRegisterSingletonInstance("org.kde.filelight", 1, 0, "ContextMenuContext", contextMenuContext);
    qmlRegisterUncreatableType<RadialMap::Segment>("org.kde.filelight", 1, 0, "Segment", QStringLiteral("only consumed, never created"));

    // Do not initialize the map too early. It causes crashes on exit. Unclear why, probably a lifetime problem deep inside the retrofitted map. May be fixable
    // with enough brain juice.
    qmlRegisterSingletonType<RadialMap::Map>("org.kde.filelight", 1, 0, "RadialMap", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        QQmlEngine::setObjectOwnership(RadialMap::Map::instance(), QQmlEngine::CppOwnership);
        return RadialMap::Map::instance();
    });
    qmlRegisterSingletonType<Config>("org.kde.filelight", 1, 0, "Config", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        QQmlEngine::setObjectOwnership(Config::instance(), QQmlEngine::CppOwnership);
        return Config::instance();
    });

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

    engine->setInitialProperties({
        {QStringLiteral("inSandbox"),
         !QStandardPaths::locate(QStandardPaths::RuntimeLocation, QStringLiteral("flatpak-info")).isEmpty() || qEnvironmentVariableIsSet("SNAP")},
    });

    const QUrl mainUrl(QStringLiteral("qrc:/ui/main.qml"));
    QObject::connect(
        engine,
        &QQmlApplicationEngine::objectCreated,
        this,
        [mainUrl](QObject *obj, const QUrl &objUrl) {
            if (!obj && mainUrl == objUrl) {
                qWarning() << "Failed to load QML dialog.";
                abort();
            }
        },
        Qt::QueuedConnection);
    engine->load(mainUrl);
}

void MainContext::scan(const QUrl &u)
{
    slotScanUrl(u);
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

} // namespace Filelight

#include "mainContext.moc"
