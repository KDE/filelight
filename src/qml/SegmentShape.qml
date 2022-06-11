// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kcoreaddons 1.0 as KCoreAddons
import QtQuick.Shapes 1.15

import org.kde.filelight 1.0

// Different from CenterShape because this lines rather than moving from center. This produces consistent strokes
// on all edges.
// NOTE: keep this file in sync with CenterShape.qml!
Shape {
    id: shape

    required property var segment

    property alias item: path.item
    property alias radius: path.radius
    property alias startAngle: path.startAngle
    property alias sweepAngle: path.sweepAngle

    property alias tooltipText: tooltip.text
    property alias showTooltip: tooltip.visible

    property alias fillColor: path.fillColor

    property var segmentUuid: segment.uuid
    property url url: segment.url()

    containsMode: Shape.FillContains

    QQC2.ToolTip {
        id: tooltip
        delay: 0
        parent: shape
        x: mouseyX + Kirigami.Units.gridUnit // offset away from mouse lest it covers the tooltip
        y: mouseyY + Kirigami.Units.gridUnit
        visible: showTooltip
    }

    ShapePath {
        id: path
        property var item: shape.item
        property var radius: shape.radius
        required property var startAngle
        required property var sweepAngle

        strokeColor: Kirigami.Theme.textColor
        strokeWidth: Kirigami.Units.smallSpacing / 4
        capStyle: ShapePath.FlatCap

        startX: item.width / 2
        startY: item.height / 2

        PathAngleArc {
            id: arc
            moveToStart: false // draw line from startX/Y to start of arc
            centerX: item.width / 2
            centerY: item.height / 2
            // minus strokewidth so it doesn't overlap the edge for the outer most segements
            radiusX: path.radius - path.strokeWidth
            radiusY: radiusX
            startAngle: path.startAngle
            sweepAngle: path.sweepAngle
        }

        PathLine { // draw line from end of arc to center again
            x: item.width / 2
            y: item.height / 2
        }
    }
}
