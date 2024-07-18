// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.config as KConfig

import org.kde.filelight 1.0

Kirigami.ApplicationWindow {
    id: appWindow

    required property bool inSandbox

    property string status
    property var mapPage: null

    title: MainContext.prettyUrl(MainContext.url)
    pageStack.globalToolBar.showNavigationButtons: {
        if (pageStack.currentItem instanceof MapPage) {
            return Kirigami.ApplicationHeaderStyle.NoNavigationButtons
        }
        return (Kirigami.ApplicationHeaderStyle.ShowBackButton | Kirigami.ApplicationHeaderStyle.ShowForwardButton)
    }

    KConfig.WindowStateSaver {
        configGroupName: "Main"
    }

    WindowThemer {
        // Kirigami.ApplicationWindow places us inside a contentItem so we can't implicitly rely on parent here, use
        // a property instead.
        window: appWindow
    }


    Kirigami.Action {
        id: scanHomeAction
        icon.name: "user-home"
        text: i18nc("@action Scan home folder", "Home Folder")
        onTriggered: appWindow.slotScanHomeFolder()
    }

    Kirigami.Action {
        id: scanRootAction
        icon.name: "folder-root"
        text: i18nc("@action Scan root folder", "Root Folder")
        onTriggered: appWindow.slotScanRootFolder()
    }

    Kirigami.Action {
        id: scanFolderAction
        icon.name: "folder"
        text: appWindow.inSandbox ? i18nc("@action Scan a user-selected folder", "Choose Folder…") : i18nc("@action Scan another folder", "Other Folder…")
        onTriggered: appWindow.slotScanFolder()
    }

    Kirigami.Action {
        id: quitAction
        icon.name: "application-exit"
        text: i18nc("@action", "Quit")
        onTriggered: Qt.quit()
    }

    Kirigami.Action {
        id: configureAction
        displayHint: Kirigami.DisplayHint.AlwaysHide
        icon.name: "configure"
        text: i18nc("@action configure app", "Configure…")
        onTriggered: {
            const openDialogWindow = pageStack.pushDialogLayer("qrc:/ui/SettingsPage.qml", {
                width: appWindow.width
            }, {
                title: i18nc("@title:window", "Configure"),
                width: Kirigami.Units.gridUnit * 30,
                height: Kirigami.Units.gridUnit * 25
            });
        }
        shortcut: "Ctrl+Shift+,"
    }

    Kirigami.Action {
        id: helpAction
        displayHint: Kirigami.DisplayHint.AlwaysHide
        icon.name: "help-browser"
        text: i18nc("@action", "Open Handbook")
        onTriggered: { Qt.openUrlExternally("help:/filelight") }
    }

    Kirigami.Action {
        id: aboutAction
        displayHint: Kirigami.DisplayHint.AlwaysHide
        icon.name: "filelight"
        text: i18nc("@action opens about app page", "About")
        onTriggered: { pageStack.layers.push("qrc:/ui/AboutPage.qml") }
    }

    GlobalMenu {}

    footer: QQC2.Control { // gives us padding and whatnot
        background: Rectangle {
            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.Header
            color: Kirigami.Theme.backgroundColor
        }

        contentItem: RowLayout {
            QQC2.Label {
                Layout.fillWidth: true
                text: appWindow.status
                elide: Text.ElideLeft
            }
            QQC2.Label {
                text: (RadialMap.numberOfChildren === 0) ?
                        i18nc("@info:status", "No files.") :
                        i18ncp("@info:status", "1 file", "%1 files", RadialMap.numberOfChildren)
            }
        }
    }

    minimumWidth: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.gridUnit * 22
    minimumHeight: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.gridUnit * 22

    width: Kirigami.Units.gridUnit * 40
    height: Kirigami.Units.gridUnit * 30

    pageStack.initialPage: "qrc:/ui/OverviewPage.qml"
    pageStack.defaultColumnWidth: appWindow.width // show single page

    signal completed

    function makeMap() {
        if (mapPage === null) {
            // Create a completely new page...
            // Kirigami is a bit smart with cleanup where it will delete mapPage when we pop it and it has no parent.
            // Explicitly create the object with appWindow as parent so this does not happen!
            const component = Qt.createComponent("qrc:/ui/MapPage.qml")
            const page = component.createObject(appWindow)
            if (component.status === component.Error) {
                throw new Error("Error while loading page: " + component.errorString());
            }
            pageStack.push(page) // MapPage sets itself as mapPage property
        } else if (mapPage !== pageStack.currentItem) {
            // If we aren't already on the page, push the existing mapPage to make sure we have a sane stack
            pageStack.pop(pageStack.initialPage)
            pageStack.push(mapPage)
        }
    }

    function slotScanFolder() {
        makeMap()
        MainContext.slotScanFolder()
    }

    function slotScanUrl(url) {
        makeMap()
        MainContext.slotScanUrl(url)
    }

    function slotScanHomeFolder() {
        makeMap()
        MainContext.slotScanHomeFolder()
    }

    function slotScanRootFolder() {
        makeMap()
        MainContext.slotScanRootFolder()
    }

    function slotUp() {
        makeMap()
        MainContext.slotUp()
    }

    function updateURL(url) {
        MainContext.updateURL(url)
    }

    function openURL(url) {
        MainContext.openUrl(url)
    }

    function rescan() {
        ScanManager.emptyCache()
        MainContext.start(MainContext.url)
    }

    function closeURL() {
        mapPage = null
        pageStack.pop(pageStack.initialPage)
        if (ScanManager.abort()) {
            appWindow.status = i18nc("@info:status", "Aborting Scan...")
        }
    }

    function rescanSingleDir(url) {
        MainContext.rescanSingleDir(url)
    }

    Connections {
        target: ScanManager

        function onRunningChanged() {
            // for when the url was set through the c++ side, e.g. as cmdline arg
            if (ScanManager.running) {
                makeMap()
            }
        }

        function onAborted() {
            appWindow.title = ""
            appWindow.status = ""
        }
    }
}
