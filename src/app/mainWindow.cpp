/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
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

#include "mainWindow.h"
#include "part/part.h"
#include "historyAction.h"

#include <cstdlib>            //std::exit()
#include <QApplication>     //setupActions()
#include <KComboBox>        //locationbar
#include <KHistoryComboBox>
#include <KRecentFilesAction>
#include <QFileDialog>
#include <KConfig>
#include <KEditToolBar>     //for editToolbar dialog
#include <QLineEdit>
#include <KStandardShortcut>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KMessageBox>
#include <KShell>
#include <QStatusBar>
#include <KToolBar>
#include <QUrl>
#include <KUrlCompletion>   //locationbar
#include <QObject>
#include <QToolTip>
#include <QPluginLoader>
#include <KConfigGroup>
#include <KShortcutsDialog>
#include <KSharedConfig>
#include <KStandardAction>
#include <KActionCollection>
#include <KIO/Global> // upUrl
#include <KService>

namespace Filelight {

MainWindow::MainWindow() : KParts::MainWindow(), m_part(0), m_histories(0)
{
//     setXMLFile("filelightui.rc");
    KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("filelightpart"));
    if (!service) {
        KMessageBox::error(this, tr("Unable to locate the Filelight Part.\nPlease make sure Filelight was correctly installed."));
        std::exit(1);
        return;
    }

    KPluginFactory *factory = KPluginLoader(service->library()).factory();

    if (!factory) {
        KMessageBox::error(this, tr("Unable to load the Filelight Part.\nPlease make sure Filelight was correctly installed."));
        std::exit(1);
        return;
    }

    m_part = static_cast<Part *>(factory->create<KParts::ReadOnlyPart>(this));

    if (m_part) {
        setStandardToolBarMenuEnabled(true);
        setupActions();
        createGUI(m_part);
        setCentralWidget(m_part->widget());

        stateChanged(QStringLiteral( "scan_failed" )); //bah! doesn't affect the parts' actions, should I add them to the actionCollection here?

        connect(m_part, SIGNAL(started(KIO::Job*)), SLOT(scanStarted()));
        connect(m_part, SIGNAL(completed()), SLOT(scanCompleted()));
        connect(m_part, SIGNAL(canceled(QString)), SLOT(scanFailed()));

        connect(m_part, SIGNAL(canceled(QString)), m_histories, SLOT(stop()));
        connect(BrowserExtension::childObject(m_part), SIGNAL(openUrlNotify()), SLOT(urlAboutToChange()));

        const KConfigGroup config = KSharedConfig::openConfig()->group("general");
        m_combo->setHistoryItems(config.readPathEntry("comboHistory", QStringList()));
    } else {
        KMessageBox::error(this, tr("Unable to create Filelight part widget.\nPlease ensure that Filelight is correctly installed."));
        std::exit(1);
    }

    setAutoSaveSettings(QStringLiteral( "window" ));
}

void MainWindow::setupActions() //singleton function
{
    KActionCollection *const ac = actionCollection();

    m_combo = new KHistoryComboBox(this);
    m_combo->setCompletionObject(new KUrlCompletion(KUrlCompletion::DirCompletion));
    m_combo->setAutoDeleteCompletionObject(true);
    m_combo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    m_combo->setDuplicatesEnabled(false);

    KStandardAction::open(this, SLOT(slotScanFolder()), ac);
    KStandardAction::quit(this, SLOT(close()), ac);
    KStandardAction::up(this, SLOT(slotUp()), ac);
    KStandardAction::configureToolbars(this, SLOT(configToolbars()), ac);
    KStandardAction::keyBindings(this, SLOT(configKeys()), ac);

    QAction* action;

    action = ac->addAction(QStringLiteral("scan_home"), this, SLOT(slotScanHomeFolder()));
    action->setText(tr("Scan &Home Folder"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("user-home")));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Home));

    action = ac->addAction(QStringLiteral("scan_root"), this, SLOT(slotScanRootFolder()));
    action->setText(tr("Scan &Root Folder"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("folder-red")));

    action = ac->addAction(QStringLiteral("scan_rescan"), m_part, SLOT(rescan()));
    action->setText(tr("Rescan"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    ac->setDefaultShortcut(action, QKeySequence::Refresh);


    action = ac->addAction(QStringLiteral("scan_stop"), this, SLOT(slotAbortScan()));
    action->setText(tr("Stop"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    ac->setDefaultShortcut(action, Qt::Key_Escape);

    action = ac->addAction(QStringLiteral("go"), m_combo, SIGNAL(returnPressed()));
    action->setText(tr("Go"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-jump-locationbar")));

    action = ac->addAction(QStringLiteral( "scan_folder" ), this, SLOT(slotScanFolder()));
    action->setText(tr("Scan Folder"));
    action->setIcon(QIcon::fromTheme(QStringLiteral( "folder" )));

    QWidgetAction *locationAction = ac->add<QWidgetAction>(QStringLiteral("location_bar"), 0, 0);
    locationAction->setText(tr("Location Bar"));
    locationAction->setDefaultWidget(m_combo);

    m_recentScans = new KRecentFilesAction(tr("&Recent Scans"), ac);
    m_recentScans->setMaxItems(8);

    m_histories = new HistoryCollection(ac, this);

    m_recentScans->loadEntries(KSharedConfig::openConfig()->group("general"));

    connect(m_recentScans, SIGNAL(urlSelected(QUrl)), SLOT(slotScanUrl(QUrl)));
    connect(m_combo, SIGNAL(returnPressed()), SLOT(slotComboScan()));
    connect(m_histories, SIGNAL(activated(QUrl)), SLOT(slotScanUrl(QUrl)));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    KConfigGroup config = KSharedConfig::openConfig()->group("general");

    m_recentScans->saveEntries(config);
    config.writePathEntry("comboHistory", m_combo->historyItems());
    config.sync();

    KParts::MainWindow::closeEvent(event);
}

void MainWindow::configToolbars() //slot
{
    KEditToolBar dialog(factory(), this);

    if (dialog.exec()) //krazy:exclude=crashy
    {
        createGUI(m_part);
        applyMainWindowSettings(KSharedConfig::openConfig()->group("window"));
    }
}

void MainWindow::configKeys() //slot
{
    KShortcutsDialog::configure(actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this, true);
}

void MainWindow::slotScanFolder()
{
    slotScanUrl(QFileDialog::getExistingDirectoryUrl(this, tr("Select Folder to Scan"), m_part->url()));
}

void MainWindow::slotScanHomeFolder() {
    slotScanPath(QDir::homePath());
}
void MainWindow::slotScanRootFolder() {
    slotScanPath(QDir::rootPath());
}
void MainWindow::slotUp()                {
    slotScanUrl(KIO::upUrl(m_part->url()));
}

void MainWindow::slotComboScan()
{
    QString path = m_combo->lineEdit()->text();

    QUrl url = QUrl::fromUserInput(path);

    if (url.isRelative())
        path = QStringLiteral( "~/" ) + path; // KUrlCompletion completes relative to ~, not CWD

    path = KShell::tildeExpand(path);

    if (slotScanPath(path))
        m_combo->addToHistory(path);
}

bool MainWindow::slotScanPath(const QString &path)
{
    return slotScanUrl(QUrl::fromUserInput(path));
}

bool MainWindow::slotScanUrl(const QUrl &url)
{
    const QUrl oldUrl = m_part->url();

    if (m_part->openUrl(url))
    {
        m_histories->push(oldUrl);
        return true;
    }
    else
        return false;
}

void MainWindow::slotAbortScan()
{
    if (m_part->closeUrl()) action("scan_stop")->setEnabled(false);
}

void MainWindow::scanStarted()
{
    stateChanged(QStringLiteral( "scan_started" ));
    m_combo->clearFocus();
}

void MainWindow::scanFailed()
{
    stateChanged(QStringLiteral( "scan_failed" ));
    action("go_up")->setStatusTip(QString());
    action("go_up")->setToolTip(QString());
    m_combo->lineEdit()->clear();
}

void MainWindow::scanCompleted()
{
    const QUrl url = m_part->url();

    stateChanged(QStringLiteral("scan_complete"));

    m_combo->lineEdit()->setText(m_part->prettyUrl());

    if (url.toLocalFile() == QLatin1String( "/" )) {
        action("go_up")->setEnabled(false);
        action("go_up")->setStatusTip(QString());
        action("go_up")->setToolTip(QString());
    }
    else {
        action("go_up")->setStatusTip(KIO::upUrl(url).toString());
        action("go_up")->setToolTip(KIO::upUrl(url).toString());
    }

    m_recentScans->addUrl(url); //FIXME doesn't set the tick
}

void MainWindow::urlAboutToChange()
{
    //called when part's URL is about to change internally
    //the part will then create the Map and emit completed()

    m_histories->push(m_part->url());
}


/**********************************************
  SESSION MANAGEMENT
 **********************************************/

void MainWindow::saveProperties(KConfigGroup &configgroup) //virtual
{
    if (!m_histories) return;

    m_histories->save(configgroup);
    configgroup.writeEntry("currentMap", m_part->url().path());
}

void MainWindow::readProperties(const KConfigGroup &configgroup) //virtual
{
    m_histories->restore(configgroup);
    slotScanPath(configgroup.group("General").readEntry("currentMap", QString()));
}

} //namespace Filelight


