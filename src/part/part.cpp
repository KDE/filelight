/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <sandsmark@samfundet.no>
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
#include <KAction>
#include <KActionCollection>
#include <KLocale>
#include <KMessageBox>  //::start()
#include <KStandardAction>
#include <KStatusBar>
#include <KPluginFactory>

#include <QtCore/QFile>        //encodeName()
#include <QtCore/QTimer>       //postInit() hack
#include <QtCore/QByteArray>
#include <QtCore/QDir>

#include <unistd.h>       //access()
#include <iostream>

namespace Filelight {

K_PLUGIN_FACTORY(filelightPartFactory, registerPlugin<Part>();)  // produce a factory
K_EXPORT_PLUGIN(filelightPartFactory(KAboutData(
               "filelightpart",
               0,
               ki18n("Filelight"),
               APP_VERSION,
               ki18n("Displays file usage in an easy to understand way."),
               KAboutData::License_GPL,
               ki18n("(c) 2002-2004 Max Howell\n\
                (c) 2008-2009 Martin T. Sandsmark"),
               KLocalizedString(),
               "http://utils.kde.org/projects/filelight",
               "sandsmark@iskrembilen.com").
               setProgramIconName(QLatin1String( "filelight" )).
               setCatalogName( "filelight" )))

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
    setComponentData(filelightPartFactory::componentData());
    setXMLFile(QLatin1String( "filelightpartui.rc" ));

    setWidget(new QWidget(parentWidget));
    widget()->setBackgroundRole(QPalette::Base);
    widget()->setAutoFillBackground(true);

    m_layout = new QGridLayout(widget());
    widget()->setLayout(m_layout);

    m_manager = new ScanManager(widget());

    m_map = new RadialMap::Widget(widget());
    m_layout->addWidget(m_map);

    m_stateWidget = new ProgressBox(statusBar(), this, m_manager);
    m_layout->addWidget(m_stateWidget);
    m_stateWidget->hide();

    m_numberOfFiles = new QLabel();
    m_statusbar->addStatusBarItem(m_numberOfFiles, 0, true);

    KStandardAction::zoomIn(m_map, SLOT(zoomIn()), actionCollection());
    KStandardAction::zoomOut(m_map, SLOT(zoomOut()), actionCollection());
    KAction *action = actionCollection()->addAction(QLatin1String("configure_filelight"));
    action->setText(i18n("Configure Filelight..."));
    action->setIcon(KIcon(QLatin1String("configure")));
    connect(action, SIGNAL(triggered()), this, SLOT(configFilelight()));

    connect(m_map, SIGNAL(created(const Folder*)), SIGNAL(completed()));
    connect(m_map, SIGNAL(created(const Folder*)), SLOT(mapChanged(const Folder*)));
    connect(m_map, SIGNAL(activated(const KUrl&)), SLOT(updateURL(const KUrl&)));

    // TODO make better system
    connect(m_map, SIGNAL(giveMeTreeFor(const KUrl&)), SLOT(updateURL(const KUrl&)));
    connect(m_map, SIGNAL(giveMeTreeFor(const KUrl&)), SLOT(openUrl(const KUrl&)));

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
Part::openUrl(const KUrl &u)
{

    //TODO everyone hates dialogs, instead render the text in big fonts on the Map
    //TODO should have an empty KUrl until scan is confirmed successful
    //TODO probably should set caption to QString::null while map is unusable

#define KMSG(s) KMessageBox::information(widget(), s)

    KUrl uri = u;
    uri.cleanPath(KUrl::SimplifyDirSeparators);
    const QString path = uri.path(KUrl::AddTrailingSlash);
    const QByteArray path8bit = QFile::encodeName(path);
    const bool isLocal = uri.protocol() == QLatin1String( "file" );

    if (uri.isEmpty())
    {
        //do nothing, chances are the user accidently pressed ENTER
    }
    else if (!uri.isValid())
    {
        KMSG(i18n("The entered URL cannot be parsed; it is invalid."));
    }
    else if ((!isLocal && path[0] != QLatin1Char( '/' )) || (isLocal && !QDir::isAbsolutePath(path)))
    {
        KMSG(i18n("Filelight only accepts absolute paths, eg. /%1", path));
    }
    else if (isLocal && access(path8bit, F_OK) != 0) //stat(path, &statbuf) == 0
    {
        KMSG(i18n("Folder not found: %1", path));
    }
    else if (isLocal && !QDir(path).isReadable()) //access(path8bit, R_OK | X_OK) != 0 doesn't work on win32
    {
        KMSG(i18n("Unable to enter: %1\nYou do not have access rights to this location.", path));
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
        statusBar()->showMessage(i18n("Aborting Scan..."));

    m_map->hide();
    m_stateWidget->hide();

    showSummary();

    return ReadOnlyPart::closeUrl();
}

void
Part::updateURL(const KUrl &u)
{
    //the map has changed internally, update the interface to reflect this
    emit m_ext->openUrlNotify(); //must be done first
    emit m_ext->setLocationBarUrl(u.prettyUrl());

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
Part::start(const KUrl &url)
{
    if (!m_started) {
        connect(m_map, SIGNAL(mouseHover(const QString&)), statusBar(), SLOT(message(const QString&)));
        connect(m_map, SIGNAL(created(const Folder*)), statusBar(), SLOT(clear()));
        m_started = true;
    }

    if (m_manager->running())
        m_manager->abort();

    m_numberOfFiles->setText(QString());

    if (m_manager->start(url)) {
        setUrl(url);

        const QString s = i18n("Scanning: %1", prettyUrl());
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
        statusBar()->showMessage(i18n("Scan completed, generating map..."));

        m_stateWidget->hide();
        m_map->show();
        m_map->create(tree);

        stateChanged(QLatin1String( "scan_complete" ));
    }
    else {
        stateChanged(QLatin1String( "scan_failed" ));
        emit canceled(i18n("Scan failed: %1", prettyUrl()));
        emit setWindowCaption(QString());

        statusBar()->clearMessage();

        setUrl(KUrl());
    }
}

void
Part::mapChanged(const Folder *tree)
{
    //IMPORTANT -> url() has already been set

    emit setWindowCaption(prettyUrl());

    const int fileCount = tree->children();
    const QString text = ( fileCount == 0 ) ?
        i18n("No files.") :
        i18np("1 file", "%1 files",fileCount);

    m_numberOfFiles->setText(text);
}

void
Part::showSummary()
{
    if (m_summary == 0) {
        m_summary = new SummaryWidget(widget());
        m_summary->setObjectName(QLatin1String( "summaryWidget" ));
        connect(m_summary, SIGNAL(activated(const KUrl&)), SLOT(openUrl(const KUrl&)));
        m_summary->show();
        m_layout->addWidget(m_summary);
    }
    else m_summary->show();
}

} //namespace Filelight

#include "part.moc"
