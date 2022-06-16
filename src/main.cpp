/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2014 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "define.h"
#include "mainContext.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QQuickStyle>
#include <QUrl>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include "fileTree.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    /**
     * enable high dpi support
     */
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif
    QApplication app(argc, argv);

    qRegisterMetaType<std::shared_ptr<File>>("std::shared_ptr<File>");
    qRegisterMetaType<std::shared_ptr<Folder>>("std::shared_ptr<Folder>");

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    KLocalizedString::setApplicationDomain("filelight");
    auto config = KSharedConfig::openConfig();
    auto stateConfig = KSharedConfig::openStateConfig();
    if (config->hasGroup("general")) {
        auto grp = stateConfig->group("general");
        config->group("general").copyTo(&grp);
        config->deleteGroup("general");
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
                     QStringLiteral("https://utils.kde.org/projects/filelight"));
    about.addAuthor(i18n("Martin Sandsmark"), i18n("Maintainer"), QStringLiteral("martin.sandsmark@kde.org"), QStringLiteral("https://iskrembilen.com/"));
    about.addAuthor(i18n("Harald Sitter"), i18n("QtQuick Port"), QStringLiteral("sitter@kde.org"));
    about.addAuthor(i18n("Max Howell"), i18n("Original author"), QStringLiteral("max.howell@methylblue.com"), QStringLiteral("https://www.methylblue.com/"));
    about.addCredit(i18n("Lukas Appelhans"), i18n("Help and support"));
    about.addCredit(i18n("Steffen Gerlach"), i18n("Inspiration"), QString(), QStringLiteral("http://www.steffengerlach.de/"));
    about.addCredit(i18n("Mike Diehl"), i18n("Original documentation"));
    about.addCredit(i18n("Sune Vuorela"), i18n("Icon"));
    about.addCredit(i18n("Nuno Pinheiro"), i18n("Icon"));

    KAboutData::setApplicationData(about);
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
