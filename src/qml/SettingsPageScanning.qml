// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.filelight 1.0

Kirigami.Page {
    Timer {
        id: cacheResetTimer
        running: false
        interval: 1000
        repeat: false
        onTriggered: ScanManager.emptyCache()
    }

    ColumnLayout {
        anchors.fill: parent
        QQC2.Label {
            text: i18nc("@label", "Do not scan these folders:")
        }
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false
            Component.onCompleted: background.visible = true

            ListView {
                id: skipView
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                reuseItems: true
                activeFocusOnTab: true
                keyNavigationEnabled: true
                keyNavigationWraps: true
                model: Config.skipList
                delegate: Kirigami.SwipeListItem {
                    QQC2.Label {
                        text: modelData
                    }
                    actions: [
                        Kirigami.Action {
                            text: i18nc("@action:button remove list entry", "Remove")
                            iconName: "list-remove"
                            onTriggered: {
                                Config.removeFolder(modelData)
                            }
                        }
                    ]
                }
            }
        }

        RowLayout {
            Item { Layout.fillWidth: true }
            QQC2.Button {
                action: Kirigami.Action {
                    text: i18nc("@action:button remove list entry", "Add…")
                    iconName: "folder-open"
                    onTriggered: Config.addFolder()
                }
            }
        }
        Kirigami.Separator {
            Layout.fillWidth: true
            height: 1
        }
        QQC2.CheckBox {
            id: scanAcrossMountsBox
            text: i18nc("@checkbox", "Scan across filesystem boundaries")
            checked: Config.scanAcrossMounts
            onToggled: {
                if (Config.scanAcrossMounts === checked) {
                    return
                }
                Config.scanAcrossMounts = checked
                cacheResetTimer.restart()
            }
        }
        QQC2.CheckBox {
            text: i18nc("@checkbox", "Exclude remote filesystems")
            checked: !Config.scanRemoteMounts
            enabled: scanAcrossMountsBox.checked
            onToggled: {
                if (Config.scanRemoteMounts === !checked) {
                    return
                }
                Config.scanRemoteMounts = !checked
                cacheResetTimer.restart()
            }
        }
    }
}
