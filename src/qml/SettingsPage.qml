// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import QtQuick.Window 2.15

import org.kde.filelight 1.0

QQC2.Pane {
    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 0

        readonly property var window: Window.window

        QQC2.TabBar {
            Layout.fillWidth: true
            id: bar
            QQC2.TabButton {
                text: i18nc("@title", "Scanning")
            }
            QQC2.TabButton {
                text: i18nc("@title", "Appearance")
            }
        }

        QQC2.Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                anchors.fill: parent
                currentIndex: bar.currentIndex
                SettingsPageScanning {}
                SettingsPageAppearance {}
            }

            Connections {
                target: layout.window
                function onClosing() { Config.write() }
            }
        }
    }
}
