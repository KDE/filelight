// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.filelight 1.0

Kirigami.ApplicationWindow {
    id: appWindow

    required property bool inSandbox

    property string status
    property var mapPage: null

    title: pageStack.currentItem.url !== undefined ? pageStack.currentItem.url : ''
    pageStack.globalToolBar.showNavigationButtons: {
        if (pageStack.currentItem instanceof MapPage) {
            return Kirigami.ApplicationHeaderStyle.NoNavigationButtons
        }
        return (Kirigami.ApplicationHeaderStyle.ShowBackButton | Kirigami.ApplicationHeaderStyle.ShowForwardButton)
    }

    Kirigami.Action {
        id: scanFolderAction
        iconName: "folder"
        text: i18nc("@action", "Scan Folder")
        onTriggered: appWindow.slotScanFolder()
    }

    Kirigami.Action {
        id: scanHomeAction
        iconName: "user-home"
        text: i18nc("@action", "Scan Home Folder")
        onTriggered: appWindow.slotScanHomeFolder()
    }

    Kirigami.Action {
        id: scanRootAction
        iconName: "folder-root"
        text: i18nc("@action", "Scan Root Folder")
        onTriggered: appWindow.slotScanRootFolder()
    }

    Kirigami.Action {
        id: quitAction
        iconName: "application-exit"
        text: i18nc("@action", "Quit")
        onTriggered: Qt.quit()
    }

    Kirigami.Action {
        id: configureAction
        displayHint: Kirigami.Action.AlwaysHide
        iconName: "configure"
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
        displayHint: Kirigami.Action.AlwaysHide
        iconName: "help-browser"
        text: i18nc("@action", "Open Handbook")
        onTriggered: { Qt.openUrlExternally("help:/filelight") }
    }

    Kirigami.Action {
        id: aboutAction
        displayHint: Kirigami.Action.AlwaysHide
        iconName: "filelight"
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
                text: (RadialMap.numberOfChildren == 0) ?
                        i18nc("@info:status", "No files.") :
                        i18ncp("@info:status", "1 file", "%1 files", RadialMap.numberOfChildren)
            }
        }
    }

    minimumWidth: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.gridUnit * 22
    onMinimumWidthChanged: console.log("Kirigami.Units.gridUnit * 22 " + Kirigami.Units.gridUnit * 22)
    minimumHeight: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.gridUnit * 22

    pageStack.initialPage: "qrc:/ui/OverviewPage.qml"
    pageStack.defaultColumnWidth: appWindow.width // show single page

    signal completed

    function makeMap() {
        if (mapPage === null) {
            pageStack.push("qrc:/ui/MapPage.qml")
        } else {
            pageStack.pop(mapPage, QQC2.StackView.Immediate)
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

    function mapChanged() {
        title = MainContext.prettyUrl(MainContext.url);
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
        pageStack.clear()
        pageStack.push(pageStack.initialPage)
        if (ScanManager.abort()) {
            appWindow.status = i18nc("@info:status", "Aborting Scan...")
        }
    }

    function rescanSingleDir(url) {
        MainContext.rescanSingleDir(url)
    }

    Connections {
        target: ScanManager
        onRunningChanged: { // for when the url was set through the c++ side, e.g. as cmdline arg
            if (ScanManager.running) {
                makeMap()
            }
        }
        onAborted: {
            appWindow.title = ""
            appWindow.status = ""
        }
    }
}
