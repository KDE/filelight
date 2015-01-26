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

#include <KAboutData>
#include <QApplication>
#include <QCommandLineParser>
#include <QUrl>

static KAboutData about(
    QStringLiteral(APP_NAME),
    QObject::tr(APP_PRETTYNAME),
    QStringLiteral(APP_VERSION),
    QObject::tr("Graphical disk-usage information"),
    KAboutLicense::GPL,
    QObject::tr("(C) 2006 Max Howell\n\
        (C) 2008-2014 Martin Sandsmark"),
    QString(),
    QStringLiteral("http://utils.kde.org/projects/filelight")
);


int main(int argc, char *argv[])
{
    using Filelight::MainWindow;

    about.addAuthor(QObject::tr("Martin Sandsmark"), QObject::tr("Maintainer"), QStringLiteral("martin.sandsmark@kde.org"), QStringLiteral("http://iskrembilen.com/"));
    about.addAuthor(QObject::tr("Max Howell"),       QObject::tr("Original author"), QStringLiteral("max.howell@methylblue.com"), QStringLiteral("http://www.methylblue.com/"));
    about.addCredit(QObject::tr("Lukas Appelhans"),  QObject::tr("Help and support"));
    about.addCredit(QObject::tr("Steffen Gerlach"),  QObject::tr("Inspiration"), QString(), QStringLiteral("http://www.steffengerlach.de/"));
    about.addCredit(QObject::tr("Mike Diehl"),       QObject::tr("Original documentation"));
    about.addCredit(QObject::tr("Sune Vuorela"),     QObject::tr("Icon"));
    about.addCredit(QObject::tr("Nuno Pinheiro"),    QObject::tr("Icon"));

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral(APP_NAME));
    app.setApplicationDisplayName(QObject::tr(APP_PRETTYNAME));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setOrganizationName(QStringLiteral("KDE"));

    QCommandLineParser options;
    options.setApplicationDescription(QObject::tr("Graphical disk-usage information"));
    options.addHelpOption();
    options.addVersionOption();
    options.addPositionalArgument(QStringLiteral("url"), QObject::tr("Path or URL to scan"), QObject::tr("[url]"));
    about.setupCommandLine(&options);
    options.process(app);
    about.processCommandLine(&options);

    if (!app.isSessionRestored()) {
        MainWindow *mw = new MainWindow();

        QStringList args = options.positionalArguments();
        if (args.count() > 0) mw->scan(QUrl::fromUserInput(args.at(0)));

        mw->show();
    }
    else RESTORE(MainWindow);

    return app.exec();
}
