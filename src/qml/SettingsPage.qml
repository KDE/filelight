// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

import org.kde.filelight 1.0

QQC2.Page {
    id: root

    Connections {
        target: root.Window.window
        function onClosing() {
            Config.write();
        }
    }

    Connections {
        target: Config
        function onAddFolderFailed(reason) {
            showPassiveNotification(reason);
        }
    }

    Timer {
        id: cacheResetTimer
        running: false
        interval: 1000
        repeat: false
        onTriggered: ScanManager.emptyCache()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Kirigami.Separator { implicitWidth: root.width }

        Kirigami.FormLayout {
            Layout.margins: Kirigami.Units.largeSpacing

            QQC2.ComboBox {
                id: colorCombobox

                Kirigami.FormData.label: i18nc("@title:group", "Colors:")

                model: [
                    i18nc("@item:inlistbox Name of a color scheme to use for the graph view", "From system color scheme"),
                    i18nc("@item:inlistbox Name of a color scheme to use for the graph view", "Rainbow"),
                    i18nc("@item:inlistbox Name of a color scheme to use for the graph view", "High contrast")
                ]
                currentIndex: {
                    if (Config.scheme === Filelight.KDE) {
                        return 0;
                    } else if (Config.scheme === Filelight.Rainbow) {
                        return 1;
                    } else if (Config.scheme === Filelight.HighContrast) {
                        return 2;
                    } else {
                        console.warn("Invalid color scheme found in config file: " + Config.scheme)
                        return -1;
                    }
                }
                onActivated: {
                    if (currentIndex === 0) {
                        Config.scheme = Filelight.KDE;
                    } else if (currentIndex === 1) {
                        Config.scheme = Filelight.Rainbow;
                    } else if (currentIndex === 2) {
                        Config.scheme = Filelight.HighContrast;
                    }
                    RadialMap.refresh(Filelight.Dirty.Colors)
                }
            }
            QQC2.Slider {
                Kirigami.FormData.label: i18nc("@label:slider", "Color contrast:")

                Layout.preferredWidth: colorCombobox.implicitWidth

                from: 0
                to: 100
                value: Config.contrast
                onValueChanged: {
                    Config.contrast = value
                    RadialMap.refresh(Filelight.Dirty.Colors)
                }
            }

            Item {
                Kirigami.FormData.isSection: true
            }

            QQC2.CheckBox {
                Kirigami.FormData.label: i18nc("@title:group Show small files/show folders sidebar", "Show:")

                text: i18nc("@option:check Show small files", "Small files")
                checked: Config.showSmallFiles
                onToggled: {
                    Config.showSmallFiles = checked
                    RadialMap.refresh(Filelight.Dirty.Layout)
                }
            }
            QQC2.CheckBox {
                id: scanAcrossMountsBox

                Layout.fillWidth: true

                text: i18nc("@option:check Show files on other disks", "Files on other disks")
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
                leftPadding: scanAcrossMountsBox.indicator.width + Kirigami.Units.smallSpacing

                text: i18nc("@option:check Show files on remote filesystems", "Files on remote filesystems")
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
            QQC2.CheckBox {
                text: i18nc("@option:check Show folders sidebar", "Folders sidebar")
                checked: Config.showFoldersSidebar
                onToggled: Config.showFoldersSidebar = checked
            }
        }

        Kirigami.Separator { implicitWidth: root.width }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false

            Component.onCompleted: background.visible = true;
            background: Rectangle {
                anchors.fill: parent
                color: Kirigami.Theme.backgroundColor
            }

            contentItem: ListView {
                clip: true
                reuseItems: true
                activeFocusOnTab: true
                keyNavigationEnabled: true
                keyNavigationWraps: true
                model: Config.skipList

                headerPositioning: ListView.OverlayHeader
                header: Kirigami.InlineViewHeader {
                    width: ListView.view.width
                    text: i18nc("@title:group", "Do not scan these folders")
                    actions: [
                        Kirigami.Action {
                            text: i18nc("@action:button remove list entry", "Add…")
                            icon.name: "folder-open"
                            onTriggered: Config.addFolder()
                        }
                    ]
                }
                delegate: QQC2.ItemDelegate {
                    id: delegate

                    required property int index
                    required property string modelData

                    width: ListView.view.width
                    text: modelData

                    Kirigami.Theme.useAlternateBackgroundColor: true

                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: activeFocus || hovered && (contentItem?.truncated ?? false)
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

                    contentItem: KD.TitleSubtitleWithActions {
                        title: delegate.text
                        selected: delegate.pressed || delegate.highlighted
                        elide: Text.ElideMiddle

                        actions: [
                            Kirigami.Action {
                                text: i18nc("@action:button remove list entry", "Resume scanning this folder")
                                icon.name: "list-remove-symbolic"
                                displayHint: Kirigami.DisplayHint.IconOnly
                                tooltip: text
                                onTriggered: Config.removeFolder(delegate.modelData)
                            }
                        ]
                    }

                    onClicked: {
                        // Do not let auto-resolver prepend "qrc:"
                        const url = Qt.url(`file://${modelData}`);
                        Qt.openUrlExternally(url);
                    }
                }
            }
        }
    }
}
