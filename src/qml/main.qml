// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.filelight 1.0

import Qt.labs.platform 1.1 as Platform

Kirigami.ApplicationWindow {
    id: appWindow

    property string status
    property int numberOfFiles
    property var mapItem: null
    property var mapPage: null

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
        onTriggered: MainContext.configFilelight()
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
                text: (appWindow.numberOfFiles == 0) ?
                        i18nc("@info:status", "No files.") :
                        i18ncp("@info:status", "1 file", "%1 files", appWindow.numberOfFiles)
            }
        }
    }

    minimumWidth: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.gridUnit * 22
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

    function mapChanged(tree) {
        title = MainContext.prettyUrl(MainContext.url);
        numberOfFiles = tree.children
    }

    function updateURL(url) {
        MainContext.updateURL(url)
    }

    function openURL(url) {
        MainContext.openURL(url)
    }

    function folderScanCompleted(tree) {
        if (tree !== null) {
            status = i18nc("@info:status", "Scan completed, generating map...")
            mapItem.create(tree);
        } else {
            title = ""
            status = ""
        }
    }

    function rescan() {
        ScanManager.emptyCache()
        MainContext.start(MainContext.url)
    }

    function closeURL() {
        if (ScanManager.abort()) {
            appWindow.status = i18nc("@info:status", "Aborting Scan...")
        }
    }

    function rescanSingleDir(url) {
        MainContext.rescanSingleDir(url)
    }

    Connections {
        target: ScanManager
        onCompleted: (tree) => appWindow.folderScanCompleted(tree)
        onRunningChanged: { // for when the url was set through the c++ side, e.g. as cmdline arg
            if (ScanManager.running) {
                makeMap()
            }
        }
    }
}
