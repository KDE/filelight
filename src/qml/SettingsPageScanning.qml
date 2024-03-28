// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

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
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true

            QQC2.Label {
                Layout.fillWidth: true
                text: i18nc("@label", "Do not scan these folders:")
                wrapMode: Text.Wrap
                verticalAlignment: Text.AlignBottom
            }

            QQC2.Button {
                Layout.alignment: Qt.AlignBottom
                action: Kirigami.Action {
                    text: i18nc("@action:button remove list entry", "Add…")
                    icon.name: "folder-open"
                    onTriggered: Config.addFolder()
                }
            }
        }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false
            Component.onCompleted: background.visible = true

            ListView {
                id: skipView
                clip: true
                reuseItems: true
                activeFocusOnTab: true
                keyNavigationEnabled: true
                keyNavigationWraps: true
                model: Config.skipList
                delegate: Kirigami.SwipeListItem {
                    id: delegate

                    required property string modelData

                    text: modelData

                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: (Kirigami.Settings.tabletMode ? down : hovered) && (contentItem?.truncated ?? false)
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

                    contentItem: KD.TitleSubtitle {
                        title: delegate.text
                        selected: delegate.highlighted
                        font: delegate.font
                        elide: Text.ElideMiddle
                    }

                    actions: [
                        Kirigami.Action {
                            text: i18nc("@action:button remove list entry", "Remove")
                            icon.name: "list-remove"
                            onTriggered: {
                                Config.removeFolder(delegate.modelData)
                            }
                        }
                    ]

                    onClicked: {
                        // Do not let auto-resolver prepend "qrc:"
                        const url = Qt.url(`file://${modelData}`);
                        Qt.openUrlExternally(url);
                    }
                }
            }
        }

        QQC2.CheckBox {
            id: scanAcrossMountsBox
            Layout.fillWidth: true
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
            Layout.fillWidth: true
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
