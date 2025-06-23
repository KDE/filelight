// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

import org.kde.filelight 1.0

Kirigami.Page {
    ColumnLayout {
        width: parent.width
        QQC2.GroupBox {
            Layout.fillWidth: true
            title: i18nc("@title:group", "Color Scheme")

            QQC2.ButtonGroup {
                buttons: [rainbowButton, systemButton, highContrastButton]
            }

            ColumnLayout {
                QQC2.RadioButton {
                    id: rainbowButton
                    text: i18nc("@option:radio a color scheme variant", "Rainbow")
                    checked: Config.scheme === Filelight.Rainbow
                    onToggled: {
                        checked ? Config.scheme = Filelight.Rainbow : null
                        RadialMap.refresh(Filelight.Dirty.Colors)
                    }
                }
                QQC2.RadioButton {
                    id: systemButton
                    text: i18nc("@option:radio a color scheme variant", "System colors")
                    checked: Config.scheme === Filelight.KDE
                    onToggled: {
                        checked ? Config.scheme = Filelight.KDE : null
                        RadialMap.refresh(Filelight.Dirty.Colors)
                    }
                }
                QQC2.RadioButton {
                    id: highContrastButton
                    text: i18nc("@option:radio a color scheme variant", "High contrast")
                    checked: Config.scheme === Filelight.HighContrast
                    onToggled: {
                        checked ? Config.scheme = Filelight.HighContrast : null
                        RadialMap.refresh(Filelight.Dirty.Colors)
                    }
                }
                RowLayout {
                    QQC2.Label {
                        text: i18nc("@label:slider", "Contrast")
                    }
                    QQC2.Slider {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: Config.contrast
                        onValueChanged: {
                            Config.contrast = value
                            RadialMap.refresh(Filelight.Dirty.Colors)
                        }
                    }
                }
            }
        }

        QQC2.CheckBox {
            text: i18nc("@checkbox", "Show small files")
            checked: Config.showSmallFiles
            onToggled: {
                Config.showSmallFiles = checked
                RadialMap.refresh(Filelight.Dirty.Layout)
            }
        }

        QQC2.CheckBox {
            text: i18nc("@checkbox", "Show folders sidebar")
            checked: Config.showFoldersSidebar
            onToggled: Config.showFoldersSidebar = checked
        }
    }
}
