// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.quickcharts 1.0 as Charts

import org.kde.filelight 1.0

Kirigami.Page {
    id: page

    title: i18nc("@title", "Overview")

    ColumnLayout {
        spacing: Kirigami.Units.gridUnit
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        anchors.centerIn: parent
        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            source: "filelight"
            implicitWidth: Kirigami.Units.iconSizes.enormous
            implicitHeight: implicitWidth
        }
        Kirigami.Heading {
            Layout.alignment: Qt.AlignHCenter
            text: i18nc("@title", "Welcome to Filelight")
        }

        Flow {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: parent.width
            Layout.preferredWidth: button1.implicitWidth + spacing + button2.implicitWidth + spacing + button3.implicitWidth
            QQC2.ToolButton {
                id: button1
                icon.width: Kirigami.Units.iconSizes.huge
                icon.height: Kirigami.Units.iconSizes.huge
                display: QQC2.AbstractButton.TextUnderIcon
                action: Kirigami.Action {
                    iconName: "folder"
                    text: i18nc("@action", "Scan Folder")
                    onTriggered: appWindow.slotScanFolder()
                }
            }
            QQC2.ToolButton {
                id: button2
                icon.width: Kirigami.Units.iconSizes.huge
                icon.height: Kirigami.Units.iconSizes.huge
                display: QQC2.AbstractButton.TextUnderIcon
                action: Kirigami.Action {
                    iconName: "user-home"
                    text: i18nc("@action", "Scan Home Folder")
                    onTriggered: appWindow.slotScanHomeFolder()
                }
            }
            QQC2.ToolButton {
                id: button3
                icon.width: Kirigami.Units.iconSizes.huge
                icon.height: Kirigami.Units.iconSizes.huge
                display: QQC2.AbstractButton.TextUnderIcon
                action: Kirigami.Action {
                    iconName: "folder-root"
                    text: i18nc("@action", "Scan Root Folder")
                    onTriggered: appWindow.slotScanRootFolder()
                }
            }
        }

    }
}
