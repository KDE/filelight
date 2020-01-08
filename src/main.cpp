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

#include "define.h"
#include "mainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QUrl>
#include <QDir>

#include <KAboutData>
#include <KLocalizedString>
#include <Kdelibs4ConfigMigrator>

int main(int argc, char *argv[])
{
    /**
     * enable high dpi support
     */
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);

    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("filelight");

    Kdelibs4ConfigMigrator migrate(QStringLiteral("filelight"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("filelightrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("filelightui.rc"));
    migrate.migrate();

    using Filelight::MainWindow;

    KAboutData about(
        QStringLiteral(APP_NAME),
        i18n(APP_PRETTYNAME),
        QStringLiteral(APP_VERSION),
        i18n("Graphical disk-usage information"),
        KAboutLicense::GPL,
        i18n("(C) 2006 Max Howell\n"
             "(C) 2008-2014 Martin Sandsmark"),
        QString(),
        QStringLiteral("https://utils.kde.org/projects/filelight")
    );
    about.addAuthor(i18n("Martin Sandsmark"), i18n("Maintainer"), QStringLiteral("martin.sandsmark@kde.org"), QStringLiteral("https://iskrembilen.com/"));
    about.addAuthor(i18n("Max Howell"),       i18n("Original author"), QStringLiteral("max.howell@methylblue.com"), QStringLiteral("https://www.methylblue.com/"));
    about.addCredit(i18n("Lukas Appelhans"),  i18n("Help and support"));
    about.addCredit(i18n("Steffen Gerlach"),  i18n("Inspiration"), QString(), QStringLiteral("http://www.steffengerlach.de/"));
    about.addCredit(i18n("Mike Diehl"),       i18n("Original documentation"));
    about.addCredit(i18n("Sune Vuorela"),     i18n("Icon"));
    about.addCredit(i18n("Nuno Pinheiro"),    i18n("Icon"));

    KAboutData::setApplicationData(about);
    app.setOrganizationName(QStringLiteral("KDE"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral(APP_NAME)));

    QCommandLineParser options;
    options.addPositionalArgument(QStringLiteral("url"), i18n("Path or URL to scan"), i18n("[url]"));
    about.setupCommandLine(&options);
    options.process(app);
    about.processCommandLine(&options);

    if (!app.isSessionRestored()) {
        MainWindow *mw = new MainWindow();

        QStringList args = options.positionalArguments();
        if (args.count() > 0) {
            mw->scan(QUrl::fromUserInput(args.at(0), QDir::currentPath(), QUrl::AssumeLocalFile));
        }

        mw->show();
    }
    else kRestoreMainWindows<MainWindow>();

    return app.exec();
}
