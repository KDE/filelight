// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kcoreaddons 1.0 as KCoreAddons

import org.kde.filelight 1.0

Kirigami.Page {
    id: page

    Kirigami.Action {
        id: goUpAction
        enabled: page.state === ""
        iconName: "go-up"
        text: i18nc("@action", "Up")
        onTriggered: appWindow.slotUp()
        shortcut: "Alt+Up"
    }

    Kirigami.Action {
        id: rescanAction
        enabled: page.state !== "scanning"
        iconName: "view-refresh"
        text: i18nc("@action", "Rescan")
        onTriggered: appWindow.rescan()
        shortcut: StandardKey.Refresh
    }

    Kirigami.Action {
        id: stopAction
        enabled: page.state === "scanning"
        iconName: "process-stop"
        text: i18nc("@action", "Stop")
        onTriggered: appWindow.closeURL()
    }

    Kirigami.Action {
        id: saveSVGAction
        enabled: page.state === ""
        iconName: "document-save"
        text: i18nc("@action", "Save as SVG")
        tooltip: i18n("Save the current view as an SVG file")
        displayHint: Kirigami.Action.AlwaysHide
        onTriggered: mapItem.saveSVG()
    }

    Kirigami.Action {
        id: zoomInAction
        enabled: page.state === ""
        iconName: "zoom-in"
        text: i18nc("@action", "Zoom In")
        displayHint: Kirigami.Action.AlwaysHide
        onTriggered: mapItem.zoomIn()
        shortcut: StandardKey.ZoomIn
    }

    Kirigami.Action {
        id: zoomOutAction
        enabled: page.state === ""
        iconName: "zoom-out"
        text: i18nc("@action", "Zoom Out")
        displayHint: Kirigami.Action.AlwaysHide
        onTriggered: mapItem.zoomOut()
        shortcut: StandardKey.ZoomOut
    }

    contextualActions: MainContext.historyActions.concat([
        goUpAction,
        rescanAction,
        stopAction,
        zoomInAction,
        zoomOutAction,
        saveSVGAction,
        configureAction,
        helpAction,
        aboutAction
    ])

    RowLayout {
        anchors.fill: parent
        RadialMapItem {
            id: mapItem
            visible: page.state === ""
            Layout.fillWidth: true
            Layout.fillHeight: true

            onFolderCreated: (tree) => {
                appWindow.completed()
                appWindow.mapChanged(tree)
                appWindow.status = ""
            }
            onActivated: (url) => {
                appWindow.updateURL(url)
            }
            onGiveMeTreeFor: (url) => {
                appWindow.updateURL(url)
                appWindow.openURL(url)
            }
            onRescanRequested: (url) => {
                appWindow.rescanSingleDir(url)
            }

            onMouseHover: (path) => {
                appWindow.status = path
            }
        }
    }

    Kirigami.LoadingPlaceholder {
        id: scanPlaceholder
        visible: page.state === "scanning"
        anchors.centerIn: parent

        Timer {
            interval: 16 // = 60 fps because supposedly Qt hardcodes it all over the place
            running: parent.visible
            repeat: true
            onTriggered: {
                const files = ScanManager.files();
                const size = ScanManager.totalSize()
                scanPlaceholder.text = i18ncp("Scanned number of files and size so far", "%1 File, %2", "%1 Files, %2", String(files), KCoreAddons.Format.formatByteSize(size));
            }
        }
    }

    Kirigami.PlaceholderMessage {
        visible: page.state === "noData"
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        text: i18n("No data available")
    }

    Connections {
        target: ScanManager
        onAboutToEmptyCache: mapItem.invalidate()
    }


    Connections {
        target: MainContext
        onCanvasIsDirty: (filth) => {
            mapItem.refresh(filth)
        }
    }

    Component.onCompleted: {
        appWindow.mapItem = mapItem
        appWindow.mapPage = this
    }

    states: [
        State {
            name: "scanning"
            when: ScanManager.running
        },
        State {
            name: "noData"
            when: !mapItem.valid
        },
        State {
            name: "" // default state
        }
    ]
}
