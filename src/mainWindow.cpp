/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
* SPDX-FileCopyrightText: 2017 Harald Sitter <sitter@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "mainWindow.h"
#include "historyAction.h"
#include "Config.h"
#include "define.h"
#include "fileTree.h"
#include "progressBox.h"
#include "radialMap/widget.h"
#include "scan.h"
#include "settingsDialog.h"
#include "summaryWidget.h"

#include <cstdlib>            //std::exit()
#include <iostream>

#include <KActionCollection>
#include <KConfigGroup>
#include <KEditToolBar>     //for editToolbar dialog
#include <KHistoryComboBox>
#include <KIO/Global> // upUrl
#include <KLocalizedString>
#include <KMessageBox>  //::start()
#include <KShell>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KUrlCompletion>   //locationbar
#include <kxmlgui_version.h>

#include <QApplication>     //setupActions()
#include <QDir>
#include <QFileDialog>
#include <QScrollArea>
#include <QStatusBar>
#include <QLineEdit>

namespace Filelight {

MainWindow::MainWindow()
    : KXmlGuiWindow()
    , m_histories(nullptr)
    , m_summary(nullptr)
    , m_map(nullptr)
    , m_started(false)
{
    Config::read();

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    setCentralWidget(scrollArea);

    QWidget *partWidget = new QWidget(scrollArea);
    scrollArea->setWidget(partWidget);

    partWidget->setBackgroundRole(QPalette::Base);
    partWidget->setAutoFillBackground(true);

    m_layout = new QGridLayout();
    partWidget->setLayout(m_layout);

    m_manager = new ScanManager(partWidget);

    m_map = new RadialMap::Widget(partWidget);
    m_layout->addWidget(m_map);

    // FIXME: drop stupid nullptr argument
    m_stateWidget = new ProgressBox(statusBar(), this, m_manager);
    m_layout->addWidget(m_stateWidget);
    m_stateWidget->hide();

    m_numberOfFiles = new QLabel();
    statusBar()->addPermanentWidget(m_numberOfFiles);

    KStandardAction::zoomIn(m_map, &RadialMap::Widget::zoomIn, actionCollection());
    KStandardAction::zoomOut(m_map, &RadialMap::Widget::zoomOut, actionCollection());
    KStandardAction::preferences(this, &MainWindow::configFilelight, actionCollection());

    connect(m_map, &RadialMap::Widget::folderCreated, this, &MainWindow::completed);
    connect(m_map, &RadialMap::Widget::folderCreated, this, &MainWindow::mapChanged);
    connect(m_map, &RadialMap::Widget::activated, this, &MainWindow::updateURL);

    // TODO make better system
    connect(m_map, &RadialMap::Widget::giveMeTreeFor, this, &MainWindow::updateURL);
    connect(m_map, &RadialMap::Widget::giveMeTreeFor, this, &MainWindow::openUrl);
    connect(m_map, &RadialMap::Widget::rescanRequested, this, &MainWindow::rescanSingleDir);

    connect(m_manager, &ScanManager::completed, this, &MainWindow::folderScanCompleted);
    connect(m_manager, &ScanManager::aboutToEmptyCache, m_map, &RadialMap::Widget::invalidate);

    setStandardToolBarMenuEnabled(true);
    setupActions();
    createGUI(QStringLiteral("filelightui.rc"));

    stateChanged(QStringLiteral("scan_failed")); //bah! doesn't affect the parts' actions, should I add them to the actionCollection here?

    connect(this, &MainWindow::started, this, &MainWindow::scanStarted);
    connect(this, &MainWindow::completed, this, &MainWindow::scanCompleted);
    connect(this, &MainWindow::canceled, this, &MainWindow::scanFailed);
    connect(this, &MainWindow::canceled, m_histories, &HistoryCollection::stop);

    const KConfigGroup config = KSharedConfig::openStateConfig()->group("general");
    m_combo->setHistoryItems(config.readPathEntry("comboHistory", QStringList()));

    setAutoSaveSettings(QStringLiteral("window"));

    QTimer::singleShot(0, this, &MainWindow::postInit);
}

void MainWindow::scan(const QUrl &u)
{
    slotScanUrl(u);
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
    KStandardAction::save(this, SLOT(slotSaveSvg()), ac)->setEnabled(false);
    KStandardAction::quit(this, SLOT(close()), ac);
    KStandardAction::up(this, SLOT(slotUp()), ac);
    KStandardAction::configureToolbars(this, SLOT(configToolbars()), ac);
    KStandardAction::keyBindings(this, SLOT(configKeys()), ac);

    QAction *action;

    action = ac->addAction(QStringLiteral("scan_home"), this, &MainWindow::slotScanHomeFolder);
    action->setText(i18n("Scan &Home Folder"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("user-home")));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_Home));

    action = ac->addAction(QStringLiteral("scan_root"), this, &MainWindow::slotScanRootFolder);
    action->setText(i18n("Scan &Root Folder"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("folder-red")));

    action = ac->addAction(QStringLiteral("scan_rescan"), this, &MainWindow::rescan);
    action->setText(i18n("Rescan"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    ac->setDefaultShortcut(action, QKeySequence::Refresh);


    action = ac->addAction(QStringLiteral("scan_stop"), this, &MainWindow::slotAbortScan);
    action->setText(i18n("Stop"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    ac->setDefaultShortcut(action, Qt::Key_Escape);

    action = ac->addAction(QStringLiteral("go"), m_combo, [this]() {
        m_combo->returnPressed(QString());
    });
    action->setText(i18n("Go"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("go-jump-locationbar")));

    action = ac->addAction(QStringLiteral("scan_folder"), this, &MainWindow::slotScanFolder);
    action->setText(i18n("Scan Folder"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("folder")));

    QWidgetAction *locationAction = ac->add<QWidgetAction>(QStringLiteral("location_bar"), nullptr, nullptr);
    locationAction->setText(i18n("Location Bar"));
    locationAction->setDefaultWidget(m_combo);

    action = ac->action(QStringLiteral("file_save"));
    action->setText(i18n("Save as SVG..."));
    action->setToolTip(i18n("Save the current view as an SVG file"));

    m_recentScans = new KRecentFilesAction(i18n("&Recent Scans"), ac);
    m_recentScans->setMaxItems(8);

    m_histories = new HistoryCollection(ac, this);

    m_recentScans->loadEntries(KSharedConfig::openStateConfig()->group("general"));

    connect(m_recentScans, &KRecentFilesAction::urlSelected, this, &MainWindow::slotScanUrl);
    connect(m_combo, QOverload<const QString &>::of(&KHistoryComboBox::returnPressed), this, &MainWindow::slotComboScan);
    connect(m_histories, &HistoryCollection::activated, this, &MainWindow::slotScanUrl);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    KConfigGroup config = KSharedConfig::openStateConfig()->group("general");

    m_recentScans->saveEntries(config);
    config.writePathEntry("comboHistory", m_combo->historyItems());
    config.sync();

    KXmlGuiWindow::closeEvent(event);
}

void MainWindow::configToolbars() //slot
{
    KEditToolBar dialog(factory(), this);

    if (dialog.exec()) //krazy:exclude=crashy
    {
        createGUI(QStringLiteral("filelightui.rc"));
        applyMainWindowSettings(KSharedConfig::openConfig()->group("window"));
    }
}

void MainWindow::configKeys() //slot
{
#if KXMLGUI_VERSION < QT_VERSION_CHECK(5,84,0)
    KShortcutsDialog::configure(actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this, true);
#else
    KShortcutsDialog::showDialog(actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this);
#endif
}

void MainWindow::slotScanFolder()
{
    slotScanUrl(QFileDialog::getExistingDirectoryUrl(this, i18n("Select Folder to Scan"), url()));
}

void MainWindow::slotScanHomeFolder()
{
    slotScanPath(QDir::homePath());
}

void MainWindow::slotSaveSvg()
{
    const QString path = QFileDialog::getSaveFileName(
        this, i18nc("@title:window", "Save as SVG"), QString(),
        i18nc("filedialog type filter", "SVG Files (*.svg);;All Files(*)"));
    if (path.isEmpty()) {
        return;
    }

    m_map->saveSvg(path);
}

void MainWindow::slotScanRootFolder()
{
    slotScanPath(QDir::rootPath());
}

void MainWindow::slotUp()
{
    slotScanUrl(KIO::upUrl(url()));
}

void MainWindow::slotComboScan()
{
    QString path = m_combo->lineEdit()->text();

    QUrl url = QUrl::fromUserInput(path);

    if (url.isRelative())
        path = QLatin1String("~/") + path; // KUrlCompletion completes relative to ~, not CWD

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
    const QUrl oldUrl = this->url();

    if (openUrl(url))
    {
        m_histories->push(oldUrl);
        return true;
    }
    else
        return false;
}

void MainWindow::slotAbortScan()
{
    if (closeUrl()) action("scan_stop")->setEnabled(false);
}

void MainWindow::scanStarted()
{
    stateChanged(QStringLiteral("scan_started"));
    m_combo->clearFocus();
}

void MainWindow::scanFailed()
{
    stateChanged(QStringLiteral("scan_failed"));
    action("go_up")->setStatusTip(QString());
    action("go_up")->setToolTip(QString());
    m_combo->lineEdit()->clear();
}

void MainWindow::scanCompleted()
{
    const QUrl url = this->url();

    stateChanged(QStringLiteral("scan_complete"));

    action("file_save")->setEnabled(true);

    m_combo->lineEdit()->setText(prettyUrl(url));

    if (url.toLocalFile() == QLatin1String("/")) {
        action("go_up")->setEnabled(false);
        action("go_up")->setStatusTip(QString());
        action("go_up")->setToolTip(QString());
    }
    else {
        action("go_up")->setEnabled(true);
        action("go_up")->setStatusTip(KIO::upUrl(url).path());
        action("go_up")->setToolTip(KIO::upUrl(url).path());
    }

    m_recentScans->addUrl(url); //FIXME doesn't set the tick
}

void MainWindow::urlAboutToChange()
{
    //called when part's URL is about to change internally
    //the part will then create the Map and emit completed()

    m_histories->push(url());
}

/**********************************************
  SESSION MANAGEMENT
 **********************************************/

void MainWindow::saveProperties(KConfigGroup &configgroup) //virtual
{
    if (!m_histories) return;

    m_histories->save(configgroup);
    configgroup.writeEntry("currentMap", url().path());
}

void MainWindow::readProperties(const KConfigGroup &configgroup) //virtual
{
    m_histories->restore(configgroup);
    slotScanPath(configgroup.group("General").readEntry("currentMap", QString()));
}

void MainWindow::postInit()
{
    if (url().isEmpty()) //if url is not empty openUrl() has been called immediately after ctor, which happens
    {
        m_map->hide();
        showSummary();

        //FIXME KXMLGUI is b0rked, it should allow us to set this
        //BEFORE createGUI is called but it doesn't
        stateChanged(QStringLiteral("scan_failed"));
    }
}

bool MainWindow::openUrl(const QUrl &u)
{

    //TODO everyone hates dialogs, instead render the text in big fonts on the Map
    //TODO should have an empty QUrl until scan is confirmed successful
    //TODO probably should set caption to QString::null while map is unusable

#define KMSG(s) KMessageBox::information(widget(), s)

    QUrl uri = u.adjusted(QUrl::NormalizePathSegments);
    const QString localPath = uri.toLocalFile();
    const bool isLocal = uri.isLocalFile();

    if (uri.isEmpty())
    {
        //do nothing, chances are the user accidentally pressed ENTER
    }
    else if (!uri.isValid())
    {
        KMSG(i18n("The entered URL cannot be parsed; it is invalid."));
    }
    else if (isLocal && !QDir::isAbsolutePath(localPath))
    {
        KMSG(i18n("Filelight only accepts absolute paths, eg. /%1", localPath));
    }
    else if (isLocal && !QDir(localPath).exists())
    {
        KMSG(i18n("Folder not found: %1", localPath));
    }
    else if (isLocal && !QDir(localPath).isReadable())
    {
        KMSG(i18n("Unable to enter: %1\nYou do not have access rights to this location.", localPath));
    }
    else
    {
        //we don't want to be using the summary screen anymore
        if (m_summary != nullptr)
            m_summary->hide();

        m_stateWidget->show();
        m_layout->addWidget(m_stateWidget);

        const bool success = start(uri);
        if (success) {
            setUrl(uri);
        }
        return success;
    }

    return false;
}

bool MainWindow::closeUrl()
{
    if (m_manager->abort())
        statusBar()->showMessage(i18n("Aborting Scan..."));

    m_map->hide();
    m_stateWidget->hide();

    showSummary();

    return true;
}

QString MainWindow::prettyUrl(const QUrl &url) const {
    return url.isLocalFile() ? QDir::toNativeSeparators(url.toLocalFile()) : url.toString();
}

void MainWindow::updateURL(const QUrl &u)
{
    if (m_manager->running())
        m_manager->abort();

    if (u == url())
        m_manager->emptyCache(); //same as rescan()

    //do this last, or it breaks Konqi location bar
    setUrl(u);
}

void MainWindow::rescanSingleDir(const QUrl &dirUrl)
{
    if (m_manager->running()) {
        m_manager->abort();
    }

    m_manager->invalidateCacheFor(dirUrl);
    m_map->hide();
    m_stateWidget->show();
    start(url());
}

QUrl MainWindow::url() const
{
    return m_url;
}

void MainWindow::setUrl(const QUrl &url)
{
    m_url = url;
}

void MainWindow::configFilelight()
{
    SettingsDialog *dialog = new SettingsDialog(widget());

    connect(dialog, &SettingsDialog::canvasIsDirty, m_map, &RadialMap::Widget::refresh);
    connect(dialog, &SettingsDialog::mapIsInvalid, m_manager, &ScanManager::emptyCache);

    if (m_summary) {
        connect(dialog, &SettingsDialog::canvasIsDirty, m_summary, &SummaryWidget::canvasDirtied);
    }

    dialog->show(); //deletes itself
}

bool MainWindow::start(const QUrl &url)
{
    if (!m_started) {
        connect(m_map, &RadialMap::Widget::mouseHover,
                [&](const QString &msg) { statusBar()->showMessage(msg); });
        connect(m_map, &RadialMap::Widget::folderCreated, statusBar(), &QStatusBar::clearMessage);
        m_started = true;
    }

    if (m_manager->running())
        m_manager->abort();

    m_numberOfFiles->setText(QString());

    if (m_manager->start(url)) {
        const QString s = i18n("Scanning: %1", prettyUrl(url));
        stateChanged(QStringLiteral("scan_started"));
        Q_EMIT started(); //as a MainWindow, we have to do this
        Q_EMIT setWindowCaption(s);
        statusBar()->showMessage(s);
        m_map->hide();
        m_map->invalidate(); //to maintain ui consistency


        return true;
    }

    return false;
}

void MainWindow::rescan()
{
    if (m_summary && !m_summary->isHidden()) {
        delete m_summary;
        m_summary = nullptr;
        showSummary();
        return;
    }

    m_manager->emptyCache(); //causes canvas to invalidate
    m_map->hide();
    m_stateWidget->show();
    start(url());
}

void MainWindow::folderScanCompleted(Folder *tree)
{
    if (tree) {
        statusBar()->showMessage(i18n("Scan completed, generating map..."));

        m_stateWidget->hide();
        m_map->show();
        m_map->create(tree);

        stateChanged(QStringLiteral("scan_complete"));
    }
    else {
        stateChanged(QStringLiteral("scan_failed"));
        Q_EMIT canceled(i18n("Scan failed: %1", prettyUrl(url())));
        Q_EMIT setWindowCaption(QString());

        statusBar()->clearMessage();

        m_map->hide();
        m_stateWidget->hide();

        showSummary();

        setUrl(QUrl());
    }
}

void MainWindow::mapChanged(const Folder *tree)
{
    //IMPORTANT -> url() has already been set

    Q_EMIT setWindowCaption(prettyUrl(url()));

    const int fileCount = tree->children();
    const QString text = (fileCount == 0) ?
        i18n("No files.") :
        i18np("1 file", "%1 files",fileCount);

    m_numberOfFiles->setText(text);
}

void MainWindow::showSummary()
{
    if (m_summary == nullptr) {
        m_summary = new SummaryWidget(widget());
        m_summary->setObjectName(QStringLiteral("summaryWidget"));
        connect(m_summary, &SummaryWidget::activated, this, &MainWindow::openUrl);
        m_summary->show();
        m_layout->addWidget(m_summary);
    }
    else m_summary->show();
}

} //namespace Filelight
