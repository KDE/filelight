// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import Qt.labs.platform 1.1 as Platform

Platform.MenuItem {
    required property var action

    checkable: action.checkable
    checked: action.checked
    icon.name: action.iconName
    icon.source: action.iconSource
    shortcut: action.shortcut
    separator: action.separator
    text: action.text
    visible: action.visible
    onTriggered: action.triggered(this)
}
