// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

import org.kde.filelight 1.0

Kirigami.Page {
    id: page

    title: i18nc("@title", "Overview")

    actions: [
        configureAction,
        helpAction,
        aboutAction
    ]

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

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: i18nc("@title", "Welcome to Filelight")
            }
            QQC2.Label {
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 25
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: i18nc("@info", "Filelight analyzes disk usage so you can see what's using lots of space. Choose a folder to proceed:")
            }
        }

        Flow {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: parent.width
            Layout.preferredWidth: button1.implicitWidth + (button2.visible ? spacing + button2.implicitWidth : 0) + (button3.visible ? spacing + button3.implicitWidth : 0)
            QQC2.ToolButton {
                id: button1
                visible: !inSandbox
                icon.width: Kirigami.Units.iconSizes.huge
                icon.height: Kirigami.Units.iconSizes.huge
                display: QQC2.AbstractButton.TextUnderIcon
                action: scanHomeAction
            }
            QQC2.ToolButton {
                id: button2
                visible: !inSandbox
                icon.width: Kirigami.Units.iconSizes.huge
                icon.height: Kirigami.Units.iconSizes.huge
                display: QQC2.AbstractButton.TextUnderIcon
                action: scanRootAction
            }
            QQC2.ToolButton {
                id: button3
                icon.width: Kirigami.Units.iconSizes.huge
                icon.height: Kirigami.Units.iconSizes.huge
                display: QQC2.AbstractButton.TextUnderIcon
                action: scanFolderAction
            }
        }
    }
}
