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

    @KIRIGAMI_PAGE_ACTIONS@: [
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
        Kirigami.Heading {
            Layout.alignment: Qt.AlignHCenter
            text: i18nc("@title", "Welcome to Filelight")
        }

        Flow {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: parent.width
            Layout.preferredWidth: button1.implicitWidth + (button2.visible ? spacing + button2.implicitWidth : 0) + (button3.visible ? spacing + button3.implicitWidth : 0)
            QQC2.ToolButton {
                id: button1
                icon.width: Kirigami.Units.iconSizes.huge
                icon.height: Kirigami.Units.iconSizes.huge
                display: QQC2.AbstractButton.TextUnderIcon
                action: scanFolderAction
            }
            QQC2.ToolButton {
                id: button2
                visible: !inSandbox
                icon.width: Kirigami.Units.iconSizes.huge
                icon.height: Kirigami.Units.iconSizes.huge
                display: QQC2.AbstractButton.TextUnderIcon
                action: scanHomeAction
            }
            QQC2.ToolButton {
                id: button3
                visible: !inSandbox
                icon.width: Kirigami.Units.iconSizes.huge
                icon.height: Kirigami.Units.iconSizes.huge
                display: QQC2.AbstractButton.TextUnderIcon
                action: scanRootAction
            }
        }

    }
}
