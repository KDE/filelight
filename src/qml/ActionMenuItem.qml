// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick
import Qt.labs.platform as Platform

Platform.MenuItem {
    required property var action

    checkable: action.checkable
    checked: action.checked
    icon.name: action.icon.name
    icon.source: action.icon.source
    shortcut: action.shortcut
    separator: action.separator
    text: action.text
    visible: action.visible
    onTriggered: action.triggered(this)
}
