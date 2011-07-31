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

#include "mainWindow.h"
#include "part/part.h"
#include "historyAction.h"

#include <cstdlib>            //std::exit()
#include <KApplication>     //setupActions()
#include <KComboBox>        //locationbar
#include <KHistoryComboBox>
#include <KRecentFilesAction>
#include <KConfig>
#include <KDirSelectDialog> //slotScanFolder
#include <KEditToolBar>     //for editToolbar dialog
#include <QLineEdit>
#include <KStandardShortcut>
#include <KFileDialog>
#include <KLibLoader>
#include <KLocale>
#include <KMessageBox>
#include <KShell>
#include <KStatusBar>
#include <KToolBar>
#include <KUrl>
#include <KUrlCompletion>   //locationbar
#include <QObject>
#include <QToolTip>
#include <KGlobal>
#include <KConfigGroup>
#include <KShortcutsDialog>
#include <KSharedConfig>
#include <KStandardAction>
#include <KActionCollection>

namespace Filelight {

MainWindow::MainWindow() : KParts::MainWindow(), m_part(0)
{
//     setXMLFile("filelightui.rc");
    KPluginFactory *factory = KPluginLoader(QLatin1String( "filelightpart" )).factory();

    if (!factory) {
        KMessageBox::error(this, i18n("Unable to load the Filelight Part.\nPlease make sure Filelight was correctly installed."));
        std::exit(1);
        return;
    }

    m_part = static_cast<Part *>(factory->create<KParts::ReadOnlyPart>(this));

    if (m_part) {
        setStandardToolBarMenuEnabled(true);
        setupActions();
        createGUI(m_part);
        setCentralWidget(m_part->widget());

        stateChanged(QLatin1String( "scan_failed" )); //bah! doesn't affect the parts' actions, should I add them to the actionCollection here?

        connect(m_part, SIGNAL(started(KIO::Job*)), SLOT(scanStarted()));
        connect(m_part, SIGNAL(completed()), SLOT(scanCompleted()));
        connect(m_part, SIGNAL(canceled(QString)), SLOT(scanFailed()));

        connect(m_part, SIGNAL(canceled(QString)), m_histories, SLOT(stop()));
        connect(BrowserExtension::childObject(m_part), SIGNAL(openUrlNotify()), SLOT(urlAboutToChange()));

        const KConfigGroup config = KGlobal::config()->group("general");
        m_combo->setHistoryItems(config.readPathEntry("comboHistory", QStringList()));
    } else {
        KMessageBox::error(this, i18n("Unable to create part widget."));
        std::exit(1);
    }

    setAutoSaveSettings(QLatin1String( "window" ));
}

inline void MainWindow::setupActions() //singleton function
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

    KAction* action;

    action = ac->addAction(QLatin1String( "scan_home" ), this, SLOT(slotScanHomeFolder()));
    action->setText(i18n("Scan &Home Folder"));
    action->setIcon(KIcon(QLatin1String( "user-home" )));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Home));

    action = ac->addAction(QLatin1String( "scan_root" ), this, SLOT(slotScanRootFolder()));
    action->setText(i18n("Scan &Root Folder"));
    action->setIcon(KIcon(QLatin1String( "folder-red" )));

    action = ac->addAction(QLatin1String( "scan_rescan" ), m_part, SLOT(rescan()));
    action->setText(i18n("Rescan"));
    action->setIcon(KIcon(QLatin1String( "view-refresh" )));
    action->setShortcut(KStandardShortcut::reload());


    action = ac->addAction(QLatin1String( "scan_stop" ), this, SLOT(slotAbortScan()));
    action->setText(i18n("Stop"));
    action->setIcon(KIcon(QLatin1String( "process-stop" )));
    action->setShortcut(Qt::Key_Escape);

    action = ac->addAction(QLatin1String( "go" ), m_combo, SIGNAL(returnPressed()));
    action->setText(i18n("Go"));
    action->setIcon(KIcon(QLatin1String( "go-jump-locationbar" )));

    action = ac->addAction(QLatin1String( "location_bar" ), 0, 0);
    action->setText(i18n("Location Bar"));
    action->setDefaultWidget(m_combo);

    action = ac->addAction(QLatin1String( "scan_folder" ), this, SLOT(slotScanFolder()));
    action->setText(i18n("Scan Folder"));
    action->setIcon(KIcon(QLatin1String( "folder" )));

    m_recentScans = new KRecentFilesAction(i18n("&Recent Scans"), ac);
    m_recentScans->setMaxItems(8);

    m_histories = new HistoryCollection(ac, this);

    m_recentScans->loadEntries(KGlobal::config()->group("general"));

    connect(m_recentScans, SIGNAL(urlSelected(KUrl)), SLOT(slotScanUrl(KUrl)));
    connect(m_combo, SIGNAL(returnPressed()), SLOT(slotComboScan()));
    connect(m_histories, SIGNAL(activated(KUrl)), SLOT(slotScanUrl(KUrl)));
}

bool MainWindow::queryExit()
{
    if (!m_part) //apparently std::exit() still calls this function, and abort() causes a crash..
        return true;

    KConfigGroup config = KGlobal::config()->group("general");

    m_recentScans->saveEntries(config);
    config.writePathEntry("comboHistory", m_combo->historyItems());
    config.sync();

    return true;
}

inline void MainWindow::configToolbars() //slot
{
    KEditToolBar dialog(factory(), this);

    if (dialog.exec()) //krazy:exclude=crashy
    {
        createGUI(m_part);
        applyMainWindowSettings(KGlobal::config()->group("window"));
    }
}

inline void MainWindow::configKeys() //slot
{
    KShortcutsDialog::configure(actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this, true);
}

inline void MainWindow::slotScanFolder()
{
    slotScanUrl(KFileDialog::getExistingDirectoryUrl(m_part->url(), this, i18n("Select Folder to Scan")));
}

inline void MainWindow::slotScanHomeFolder() {
    slotScanPath(QDir::homePath());
}
inline void MainWindow::slotScanRootFolder() {
    slotScanPath(QDir::rootPath());
}
inline void MainWindow::slotUp()                {
    slotScanUrl(m_part->url().upUrl());
}

inline void MainWindow::slotComboScan()
{
    QString path = m_combo->lineEdit()->text();

    KUrl url = KUrl(path);

    if (url.isRelative())
        path = QLatin1String( "~/" ) + path; // KUrlCompletion completes relative to ~, not CWD

    path = KShell::tildeExpand(path);

    if (slotScanPath(path))
        m_combo->addToHistory(path);
}

inline bool MainWindow::slotScanPath(const QString &path)
{
    return slotScanUrl(KUrl(path));
}

bool MainWindow::slotScanUrl(const KUrl &url)
{
    const KUrl oldUrl = m_part->url();

    if (m_part->openUrl(url))
    {
        m_histories->push(oldUrl);
        return true;
    }
    else
        return false;
}

inline void MainWindow::slotAbortScan()
{
    if (m_part->closeUrl()) action("scan_stop")->setEnabled(false);
}

inline void MainWindow::scanStarted()
{
    stateChanged(QLatin1String( "scan_started" ));
    m_combo->clearFocus();
}

inline void MainWindow::scanFailed()
{
    stateChanged(QLatin1String( "scan_failed" ));
    setActionMenuTextOnly(qobject_cast<KAction *>(action("go_up")), QString());
    m_combo->lineEdit()->clear();
}

void MainWindow::scanCompleted()
{
    KAction *goUp  = qobject_cast<KAction *>(action("go_up"));
    const KUrl url = m_part->url();

    stateChanged(QLatin1String( "scan_complete" ));

    m_combo->lineEdit()->setText(m_part->prettyUrl());

    if (url.path(KUrl::LeaveTrailingSlash) == QLatin1String( "/" )) {
        goUp->setEnabled(false);
        setActionMenuTextOnly(goUp, QString());
    }
    else
        setActionMenuTextOnly(goUp, url.upUrl().path(KUrl::LeaveTrailingSlash));

    m_recentScans->addUrl(url); //FIXME doesn't set the tick
}

inline void MainWindow::urlAboutToChange()
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
    m_histories->save(configgroup);
    configgroup.writeEntry("currentMap", m_part->url().path());
}

void MainWindow::readProperties(const KConfigGroup &configgroup) //virtual
{
    m_histories->restore(configgroup);
    slotScanPath(configgroup.group("General").readEntry("currentMap", QString()));
}

} //namespace Filelight



/// declared in historyAction.h

void setActionMenuTextOnly(KAction *a, QString const &suffix)
{
    //TODO: In KDE 4.3, we have KAction::setHelpText(), which can replace this.
    QString const menu_text = suffix.isEmpty()
                              ? a->text()
                              : i18nc("&Up: /home/mxcl", "%1: %2", a->text(), suffix);

    for (int i = 0; i < a->associatedWidgets().count(); ++i) {
        QWidget *w = a->associatedWidgets().value(i);
//        int const id = a->itemId(i);

        /*if (w->inherits("QPopupMenu")) //FIXME: This was probably here for a reason!
            static_cast<Q3PopupMenu*>(w)->changeItem(id, menu_text);

        else */
        if (w->inherits("KToolBar")) {
            QWidget *button = static_cast<KToolBar*>(w);
            if (button->inherits("KToolBarButton"))
                button->setToolTip(suffix);
        }
    }
}

#include "mainWindow.moc"
