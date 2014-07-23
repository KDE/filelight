/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2013  Martin Sandsmark <martin.sandsmark@kde.org>
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

#include "part.h"

#include "Config.h"
#include "define.h"
#include "fileTree.h"
#include "progressBox.h"
#include "radialMap/widget.h"
#include "scan.h"
#include "settingsDialog.h"
#include "summaryWidget.h"

#include <KAboutData>   //::createAboutData()
#include <QAction>
#include <KActionCollection>
#include <KMessageBox>  //::start()
#include <KStandardAction>
#include <QStatusBar>
#include <KPluginFactory>

#include <QFile>        //encodeName()
#include <QTimer>       //postInit() hack
#include <QByteArray>
#include <QDir>
#include <QScrollArea>

#include <unistd.h>       //access()
#include <iostream>

namespace Filelight {

K_PLUGIN_FACTORY(filelightPartFactory, registerPlugin<Part>();)  // produce a factory

BrowserExtension::BrowserExtension(Part *parent)
        : KParts::BrowserExtension(parent)
{}


Part::Part(QWidget *parentWidget, QObject *parent, const QList<QVariant>&)
        : ReadOnlyPart(parent)
        , m_summary(0)
        , m_ext(new BrowserExtension(this))
        , m_statusbar(new StatusBarExtension(this))
        , m_map(0)
        , m_started(false)
{
    Config::read();

    KAboutData aboutData(
            QLatin1String("filelightpart"),
            tr(APP_PRETTYNAME),
            QLatin1String(APP_VERSION),
            tr("Displays file usage in an easy to understand way."),
            KAboutLicense::GPL,
            tr("(c) 2002-2004 Max Howell\n\
                (c) 2008-2014 Martin T. Sandsmark"),
            QString(),
            QLatin1String("http://utils.kde.org/projects/filelight"),
            QLatin1String("martin.sandsmark@kde.org"));
    aboutData.setProgramIconName(QLatin1String(APP_NAME));
    setComponentData(aboutData);

    setXMLFile(QLatin1String( "filelightpartui.rc" ));

    QScrollArea *scrollArea = new QScrollArea(parentWidget);
    scrollArea->setWidgetResizable(true);
    setWidget(scrollArea);

    QWidget *partWidget = new QWidget(scrollArea);
    scrollArea->setWidget(partWidget);

    partWidget->setBackgroundRole(QPalette::Base);
    partWidget->setAutoFillBackground(true);

    m_layout = new QGridLayout();
    partWidget->setLayout(m_layout);

    m_manager = new ScanManager(partWidget);

    m_map = new RadialMap::Widget(partWidget);
    m_layout->addWidget(m_map);

    m_stateWidget = new ProgressBox(statusBar(), this, m_manager);
    m_layout->addWidget(m_stateWidget);
    m_stateWidget->hide();

    m_numberOfFiles = new QLabel();
    m_statusbar->addStatusBarItem(m_numberOfFiles, 0, true);

    KStandardAction::zoomIn(m_map, SLOT(zoomIn()), actionCollection());
    KStandardAction::zoomOut(m_map, SLOT(zoomOut()), actionCollection());
    QAction *action = actionCollection()->addAction(QLatin1String("configure_filelight"));
    action->setText(tr("Configure Filelight..."));
    action->setIcon(QIcon(QLatin1String("configure")));
    connect(action, SIGNAL(triggered()), this, SLOT(configFilelight()));

    connect(m_map, SIGNAL(created(const Folder*)), SIGNAL(completed()));
    connect(m_map, SIGNAL(created(const Folder*)), SLOT(mapChanged(const Folder*)));
    connect(m_map, SIGNAL(activated(QUrl)), SLOT(updateURL(QUrl)));

    // TODO make better system
    connect(m_map, SIGNAL(giveMeTreeFor(QUrl)), SLOT(updateURL(QUrl)));
    connect(m_map, SIGNAL(giveMeTreeFor(QUrl)), SLOT(openUrl(QUrl)));

    connect(m_manager, SIGNAL(completed(Folder*)), SLOT(scanCompleted(Folder*)));
    connect(m_manager, SIGNAL(aboutToEmptyCache()), m_map, SLOT(invalidate()));

    QTimer::singleShot(0, this, SLOT(postInit()));
}

void
Part::postInit()
{
    if (url().isEmpty()) //if url is not empty openUrl() has been called immediately after ctor, which happens
    {
        m_map->hide();
        showSummary();

        //FIXME KXMLGUI is b0rked, it should allow us to set this
        //BEFORE createGUI is called but it doesn't
        stateChanged(QLatin1String( "scan_failed" ));
    }
}

bool
Part::openUrl(const QUrl &u)
{

    //TODO everyone hates dialogs, instead render the text in big fonts on the Map
    //TODO should have an empty QUrl until scan is confirmed successful
    //TODO probably should set caption to QString::null while map is unusable

#define KMSG(s) KMessageBox::information(widget(), s)

    QUrl uri = u.adjusted(QUrl::NormalizePathSegments);
    const QString path = uri.path();
    const QByteArray path8bit = QFile::encodeName(path);
    const bool isLocal = uri.scheme() == QLatin1String( "file" );

    if (uri.isEmpty())
    {
        //do nothing, chances are the user accidently pressed ENTER
    }
    else if (!uri.isValid())
    {
        KMSG(tr("The entered URL cannot be parsed; it is invalid."));
    }
    else if ((!isLocal && path[0] != QLatin1Char( '/' )) || (isLocal && !QDir::isAbsolutePath(path)))
    {
        KMSG(tr("Filelight only accepts absolute paths, eg. /%1").arg(path));
    }
    else if (isLocal && access(path8bit, F_OK) != 0) //stat(path, &statbuf) == 0
    {
        KMSG(tr("Folder not found: %1").arg(path));
    }
    else if (isLocal && !QDir(path).isReadable()) //access(path8bit, R_OK | X_OK) != 0 doesn't work on win32
    {
        KMSG(tr("Unable to enter: %1\nYou do not have access rights to this location.").arg(path));
    }
    else
    {
        //we don't want to be using the summary screen anymore
        if (m_summary != 0)
            m_summary->hide();

        m_stateWidget->show();
        m_layout->addWidget(m_stateWidget);

        return start(uri);
    }

    return false;
}

bool
Part::closeUrl()
{
    if (m_manager->abort())
        statusBar()->showMessage(tr("Aborting Scan..."));

    m_map->hide();
    m_stateWidget->hide();

    showSummary();

    return ReadOnlyPart::closeUrl();
}

void
Part::updateURL(const QUrl &u)
{
    //the map has changed internally, update the interface to reflect this
    emit m_ext->openUrlNotify(); //must be done first
    emit m_ext->setLocationBarUrl(u.toString(QUrl::PreferLocalFile));

    if (m_manager->running())
        m_manager->abort();

    if (u == url())
        m_manager->emptyCache(); //same as rescan()

    //do this last, or it breaks Konqi location bar
    setUrl(u);
}

void
Part::configFilelight()
{
    QWidget *dialog = new SettingsDialog(widget());

    connect(dialog, SIGNAL(canvasIsDirty(int)), m_map, SLOT(refresh(int)));
    connect(dialog, SIGNAL(mapIsInvalid()), m_manager, SLOT(emptyCache()));

    dialog->show(); //deletes itself
}

bool
Part::start(const QUrl &url)
{
    if (!m_started) {
        connect(m_map, SIGNAL(mouseHover(QString)), statusBar(), SLOT(message(QString)));
        connect(m_map, SIGNAL(created(const Folder*)), statusBar(), SLOT(clear()));
        m_started = true;
    }

    if (m_manager->running())
        m_manager->abort();

    m_numberOfFiles->setText(QString());

    if (m_manager->start(url)) {
        setUrl(url);

        const QString s = tr("Scanning: %1").arg(prettyUrl());
        stateChanged(QLatin1String( "scan_started" ));
        emit started(0); //as a Part, we have to do this
        emit setWindowCaption(s);
        statusBar()->showMessage(s);
        m_map->hide();
        m_map->invalidate(); //to maintain ui consistency


        return true;
    }

    return false;
}

void
Part::rescan()
{
    if (m_summary && !m_summary->isHidden()) {
        delete m_summary;
        m_summary = 0;
        showSummary();
        return;
    }

    //FIXME we have to empty the cache because otherwise rescan picks up the old tree..
    m_manager->emptyCache(); //causes canvas to invalidate
    m_map->hide();
    m_stateWidget->show();
    start(url());
}

void
Part::scanCompleted(Folder *tree)
{
    if (tree) {
        statusBar()->showMessage(tr("Scan completed, generating map..."));

        m_stateWidget->hide();
        m_map->show();
        m_map->create(tree);

        stateChanged(QLatin1String( "scan_complete" ));
    }
    else {
        stateChanged(QLatin1String( "scan_failed" ));
        emit canceled(tr("Scan failed: %1").arg(prettyUrl()));
        emit setWindowCaption(QString());

        statusBar()->clearMessage();

        setUrl(QUrl());
    }
}

void
Part::mapChanged(const Folder *tree)
{
    //IMPORTANT -> url() has already been set

    emit setWindowCaption(prettyUrl());

    const int fileCount = tree->children();
    const QString text = ( fileCount == 0 ) ?
        tr("No files.") :
        tr("%n file(s)", "number of files found", fileCount);

    m_numberOfFiles->setText(text);
}

void
Part::showSummary()
{
    if (m_summary == 0) {
        m_summary = new SummaryWidget(widget());
        m_summary->setObjectName(QLatin1String( "summaryWidget" ));
        connect(m_summary, SIGNAL(activated(QUrl)), SLOT(openUrl(QUrl)));
        m_summary->show();
        m_layout->addWidget(m_summary);
    }
    else m_summary->show();
}

} //namespace Filelight

#include "part.moc"
