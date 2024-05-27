// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022-2024 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.ToolTip {
    id: tooltip
    delay: 0

    required property real mouseX
    required property real mouseY

    // Qt messes up the coordinates when the ToolTip doesn't fit between cursor and window edge. Manually calculate
    // positions such that they don't clash and Qt has no reason to try and meddle with our coordinates.
    // https://bugreports.qt.io/browse/QTBUG-113468
    readonly property real xShowRight: mouseX + Kirigami.Units.gridUnit
    readonly property real yShowBelow: mouseY + Kirigami.Units.gridUnit
    readonly property real xShowLeft: mouseX - width - Kirigami.Units.gridUnit
    readonly property real yShowAbove: mouseY - height - Kirigami.Units.gridUnit
    x: xShowRight + width > parent.width ? xShowLeft : xShowRight
    y: yShowBelow + height > parent.height ? yShowAbove : yShowBelow
}
