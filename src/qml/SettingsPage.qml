// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import QtQuick.Window

import org.kde.filelight 1.0

QQC2.Pane {
    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 0

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
                target: layout.Window.window
                function onClosing() {
                    Config.write();
                }
            }
        }
    }
}
