import QtQuick
import QtQuick.Window

import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.Xr

import org.kde.filelight

XrView {
    id: xrView

    required property MapPage mapPage

    XrErrorDialog { id: err }
    onInitializeFailed: (errorString) => err.run("XRView", errorString)

    referenceSpace: XrView.ReferenceSpaceLocalFloor
    passthroughEnabled: true

    environment: ExtendedSceneEnvironment {
        backgroundMode: SceneEnvironment.Transparent
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
        glowEnabled: true
        glowBlendMode: ExtendedSceneEnvironment.GlowBlendMode.Additive
        glowLevel: ExtendedSceneEnvironment.GlowLevel.Two
                   | ExtendedSceneEnvironment.GlowLevel.Three
                   | ExtendedSceneEnvironment.GlowLevel.Four
                   | ExtendedSceneEnvironment.GlowLevel.Five
                   | ExtendedSceneEnvironment.GlowLevel.Six
        glowBloom: 0.35
        glowIntensity: 0.24
        glowStrength: 1.0
        glowQualityHigh: true
        glowUseBicubicUpscale: true
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

    DirectionalLight {
        eulerRotation: Qt.vector3d(-55, -25, 0)
        brightness: 0.75
        ambientColor: "#1c1c1c"
    }
    DirectionalLight {
        eulerRotation: Qt.vector3d(-25, 140, 0)
        brightness: 0.3
        color: "#c5deff"
    }

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
                        readonly property color sliceColor: segmentHover ? Qt.darker(segment.color) : segment.color

                        pickable: true

                        geometry: XrPieSliceGeometry {
                            innerRadius: pieRoot.ringWidth * (level.index + 1)
                            radius: pieRoot.ringWidth * (level.index + 2)
                            startAngle: slice.segment.start() / 16 + 90
                            sweepAngle: slice.segment.length() / 16
                            thickness: pieRoot.sliceThickness
                        }

                        materials: PrincipledMaterial {
                            baseColor: slice.sliceColor
                            metalness: 0.0
                            roughness: 0.32
                            specularAmount: 0.65
                            emissiveFactor: Qt.vector3d(slice.sliceColor.r * 0.03,
                                                        slice.sliceColor.g * 0.03,
                                                        slice.sliceColor.b * 0.03)
                        }

                        Model {
                            geometry: XrPieSliceGeometry {
                                innerRadius: pieRoot.ringWidth * (level.index + 1)
                                radius: pieRoot.ringWidth * (level.index + 2)
                                startAngle: slice.segment.start() / 16 + 90
                                sweepAngle: slice.segment.length() / 16
                                thickness: pieRoot.sliceThickness
                                edgeOnly: true
                            }
                            materials: PrincipledMaterial {
                                lighting: PrincipledMaterial.NoLighting
                                cullMode: Material.NoCulling
                                baseColor: Qt.rgba(0.55 + 0.45 * slice.sliceColor.r,
                                                   0.55 + 0.45 * slice.sliceColor.g,
                                                   0.55 + 0.45 * slice.sliceColor.b, 1)
                            }
                        }
                    }
                }
            }
        }

        Model {
            id: centerDisc
            readonly property string segmentUuid: "root"
            readonly property color sliceColor: pieRoot.hoveredSegment === "root" ? Qt.darker("#dddddd") : "#dddddd"

            pickable: true

            geometry: XrPieSliceGeometry {
                innerRadius: 0
                radius: pieRoot.ringWidth
                startAngle: 0
                sweepAngle: 360
                thickness: pieRoot.sliceThickness
            }
            materials: PrincipledMaterial {
                baseColor: centerDisc.sliceColor
                metalness: 0.0
                roughness: 0.32
                specularAmount: 0.65
                emissiveFactor: Qt.vector3d(centerDisc.sliceColor.r * 0.03,
                                            centerDisc.sliceColor.g * 0.03,
                                            centerDisc.sliceColor.b * 0.03)
            }
            Model {
                geometry: XrPieSliceGeometry {
                    innerRadius: 0
                    radius: pieRoot.ringWidth
                    startAngle: 0
                    sweepAngle: 360
                    thickness: pieRoot.sliceThickness
                    edgeOnly: true
                }
                materials: PrincipledMaterial {
                    lighting: PrincipledMaterial.NoLighting
                    cullMode: Material.NoCulling
                    baseColor: Qt.rgba(0.55 + 0.45 * centerDisc.sliceColor.r,
                                       0.55 + 0.45 * centerDisc.sliceColor.g,
                                       0.55 + 0.45 * centerDisc.sliceColor.b, 1)
                }
            }
        }
    }
}
