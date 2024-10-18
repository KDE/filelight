/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2014 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "define.h"
#include "mainContext.h"

#include <filesystem>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QQuickStyle>
#include <QUrl>

#include <KAboutData>
#include <KColorSchemeManager>
#include <KConfigGroup>
#include <KCrash>
#include <KLocalizedString>
#include <KSharedConfig>

#include "fileCleaner.h"
#include "fileTree.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    // Since filelight may get used when the disk is full or near full we'll not
    // want to risk caching problems. If we don't have enough space -> disable caching.
    // https://bugs.kde.org/show_bug.cgi?id=466415
    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    std::error_code ec;
    const auto spaceInfo = std::filesystem::space(cachePath.toStdString(), ec);
    constexpr auto minimumSpace = 5UL * 1024 * 1024 * 1024; // GiB
    const bool cache = ec ? false : (spaceInfo.available > minimumSpace);
    if (!cache) {
        qWarning() << "Disabling QML cache because of low disk space";
        qputenv("QML_DISABLE_DISK_CACHE", "1");
    }

    QApplication app(argc, argv);

    qRegisterMetaType<std::shared_ptr<File>>("std::shared_ptr<File>");
    qRegisterMetaType<std::shared_ptr<Folder>>("std::shared_ptr<Folder>");

    std::ignore = FileCleaner::instance(); // make sure the cleaner is up and running early so it is definitely up by the time shutdown happens

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

#if defined(Q_OS_WINDOWS)
    // Ensure we have a suitable color theme set for light/dark mode. KColorSchemeManager implicitly applies
    // a suitable default theme.
#if KCOLORSCHEME_VERSION < QT_VERSION_CHECK(6, 6, 0)
    KColorSchemeManager manager;
#else
    (void)KColorSchemeManager::instance();
#endif
    // Force breeze style to ensure coloring works consistently in dark mode. Specifically tab colors have
    // troubles on windows.
    app.setStyle(u"breeze"_s);
    // Force breeze icon theme to ensure we can correctly adapt icons to color changes WRT dark/light mode.
    // Without this we may end up with hicolor and fail to support icon recoloring.
    QIcon::setThemeName(u"breeze"_s);
#endif

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("filelight"));
    auto config = KSharedConfig::openConfig();
    auto stateConfig = KSharedConfig::openStateConfig();
    if (config->hasGroup(QLatin1String("general"))) {
        auto grp = stateConfig->group(QStringLiteral("general"));
        config->group(QStringLiteral("general")).copyTo(&grp);
        config->deleteGroup(QStringLiteral("general"));
    }

    KAboutData about(QStringLiteral(APP_NAME),
                     i18n(APP_PRETTYNAME),
                     QStringLiteral(APP_VERSION),
                     i18n("Graphical disk-usage information"),
                     KAboutLicense::GPL,
                     i18n("(C) 2006 Max Howell\n"
                          "(C) 2008-2014 Martin Sandsmark\n"
                          "(C) 2017-2022 Harald Sitter"),
                     QString(),
                     QStringLiteral("https://apps.kde.org/filelight"));
    about.addAuthor(i18n("Laurent Montel"), i18n("Maintainer"), QStringLiteral("montel@kde.org"));
    about.addAuthor(i18n("Martin Sandsmark"), i18n("Maintainer"), QStringLiteral("martin.sandsmark@kde.org"), QStringLiteral("https://iskrembilen.com/"));
    about.addAuthor(i18n("Harald Sitter"), i18n("QtQuick Port"), QStringLiteral("sitter@kde.org"));
    about.addAuthor(i18n("Max Howell"), i18n("Original author"), QStringLiteral("max.howell@methylblue.com"), QStringLiteral("https://www.methylblue.com/"));
    about.addCredit(i18n("Lukas Appelhans"), i18n("Help and support"));
    about.addCredit(i18n("Steffen Gerlach"), i18n("Inspiration"), QString(), QStringLiteral("http://www.steffengerlach.de/"));
    about.addCredit(i18n("Mike Diehl"), i18n("Original documentation"));
    about.addCredit(i18n("Sune Vuorela"), i18n("Icon"));
    about.addCredit(i18n("Nuno Pinheiro"), i18n("Icon"));

    KAboutData::setApplicationData(about);
    KCrash::initialize();
    app.setOrganizationName(QStringLiteral("KDE"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral(APP_NAME)));

    QCommandLineParser options;
    options.addPositionalArgument(QStringLiteral("url"), i18n("Path or URL to scan"), i18n("[url]"));
    about.setupCommandLine(&options);
    options.process(app);
    about.processCommandLine(&options);

    Filelight::MainContext mainContext;
    const QStringList args = options.positionalArguments();
    if (args.count() > 0) {
        mainContext.scan(QUrl::fromUserInput(args.at(0), QDir::currentPath(), QUrl::AssumeLocalFile));
    }

    return app.exec();
}
