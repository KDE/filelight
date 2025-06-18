// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.coreaddons 1.0 as KCoreAddons
import QtQuick.Shapes

import org.kde.filelight 1.0

// Different from SegmentShape because this moves rather than lining from center. This avoids obnoxious strokes from
// the center outwards.
// NOTE: keep this file in sync with SegmentShape.qml!
Shape {
    id: shape

    required property Segment segment

    property alias item: path.item
    property alias radius: path.radius
    property alias startAngle: path.startAngle
    property alias sweepAngle: path.sweepAngle

    property alias tooltipText: tooltip.text
    property alias showTooltip: tooltip.visible

    property color fillColor

    property string segmentUuid: segment.uuid
    property url url: segment ? segment.url() : ""

    containsMode: Shape.FillContains
    preferredRendererType: Shape.CurveRenderer
    asynchronous: true

    property bool wasReady: false

    function forceUpdate() {
        path.fillColor = "transparent"
        path.fillColor = Qt.binding(function() { return shape.fillColor })
    }

    onStatusChanged: {
        // Hack for https://bugreports.qt.io/browse/QTBUG-128637
        // Force an update by briefly switching the colors around.
        if (!wasReady && shape.status === Shape.Ready) {
            wasReady = true
            Qt.callLater(forceUpdate)
        }
    }

    ShapeToolTip {
        id: tooltip
        parent: shapeItem
        mouseX: mouseyX
        mouseY: mouseyY
    }

    ShapePath {
        id: path
        property var item: shape.item
        property var radius: shape.radius
        required property var startAngle
        required property var sweepAngle

        fillColor: shape.fillColor
        strokeColor: Kirigami.Theme.textColor
        strokeWidth: Kirigami.Units.smallSpacing / 4
        capStyle: ShapePath.FlatCap

        startX: item.width / 2
        startY: item.height / 2

        PathAngleArc {
            id: arc
            moveToStart: true // move line from startX/Y to start of arc
            centerX: item.width / 2
            centerY: item.height / 2
            // minus strokewidth so it doesn't overlap the edge for the outer most segements
            radiusX: path.radius - path.strokeWidth
            radiusY: radiusX
            startAngle: path.startAngle
            sweepAngle: path.sweepAngle
        }

        PathMove { // move line from end of arc to center again
            x: item.width / 2
            y: item.height / 2
        }
    }
}
