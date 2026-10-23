import QtQuick
import QtQuick.Window

import QtQuick3D
import QtQuick3D.Xr

import org.kde.kirigami as Kirigami

import org.kde.filelight

XrView {
    id: xrView

    required property MapPage mapPage

    XrErrorDialog { id: err }
    onInitializeFailed: (errorString) => err.run("XRView", errorString)

    referenceSpace: XrView.ReferenceSpaceLocalFloor

    environment: SceneEnvironment {
        Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
        clearColor: Kirigami.Theme.backgroundColor
        backgroundMode: SceneEnvironment.Color
    }

    xrOrigin: XrOrigin {
        id: theOrigin

        XrController {
            controller: XrController.LeftController
            poseSpace: XrController.AimPose
        }

        XrController {
            controller: XrController.RightController
            poseSpace: XrController.AimPose
        }
    }

    DirectionalLight {}

    Node {
        id: pieRoot

        position: Qt.vector3d(0, 40, -150)
        eulerRotation.x: 45

        readonly property real mapRadius: 75
        readonly property real sliceThickness: 20
        readonly property var signature: RadialMap.signature
        readonly property int levels: signature ? signature.length : 0
        readonly property real ringWidth: mapRadius / (levels + 1)

        property var hoveredSegment: mapPage.hoveredSegment

        Repeater3D {
            model: pieRoot.signature
            delegate: Node {
                id: level
                required property int index
                required property var modelData // segments for this level

                Repeater3D {
                    model: level.modelData
                    delegate: Model {
                        id: slice
                        required property var modelData
                        readonly property var segment: modelData

                        readonly property bool segmentHover: pieRoot.hoveredSegment === segment.uuid

                        pickable: true

                        geometry: XrPieSliceGeometry {
                            innerRadius: pieRoot.ringWidth * (level.index + 1)
                            radius: pieRoot.ringWidth * (level.index + 2)
                            startAngle: slice.segment.start() / 16 + 90
                            sweepAngle: slice.segment.length() / 16
                            thickness: pieRoot.sliceThickness
                        }

                        materials: PrincipledMaterial {
                            baseColor: slice.segmentHover ? Qt.darker(slice.segment.color) : slice.segment.color
                            roughness: 0.4
                        }
                    }
                }
            }
        }

        Model {
            id: centerDisc
            readonly property string segmentUuid: "root"

            pickable: true

            geometry: XrPieSliceGeometry {
                innerRadius: 0
                radius: pieRoot.ringWidth
                startAngle: 0
                sweepAngle: 360
                thickness: pieRoot.sliceThickness
            }
            materials: PrincipledMaterial {
                baseColor: pieRoot.hoveredSegment === "root" ? Qt.darker("#dddddd") : "#dddddd"
                roughness: 0.4
            }
        }

        /*
        ListModel {
            id: pieData

            ListElement {
                innerRadius: 35
                radius: 75
                startAngle: 0
                sweepAngle: 70
                thickness: 20
                color: "#42a5f5"
            }
            ListElement {
                innerRadius: 35
                radius: 75
                startAngle: 70
                sweepAngle: 120
                thickness: 20
                color: "yellow"
            }
            ListElement {
                innerRadius: 35
                radius: 75
                startAngle: 190
                sweepAngle: 170
                thickness: 20
                color: "green"
            }
        }

        Repeater3D {
            model: pieData
            delegate: Model {
                id: pieDelegate

                required property int innerRadius
                required property int radius
                required property int startAngle
                required property int sweepAngle
                required property int thickness
                required property color color

                geometry: XrPieSliceGeometry {
                    innerRadius: pieDelegate.innerRadius
                    radius: pieDelegate.radius
                    startAngle: pieDelegate.startAngle
                    sweepAngle: pieDelegate.sweepAngle
                    thickness: pieDelegate.thickness
                }

                materials: [
                    PrincipledMaterial {
                        baseColor: pieDelegate.color
                        roughness: 0.4
                    }
                ]
            }
        }
        */
    }
}
