#pragma once

#include <QtQuick3D/QQuick3DGeometry>
#include <QtQml/qqmlregistration.h>

class XrPieSliceGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    QML_NAMED_ELEMENT(XrPieSliceGeometry)

    Q_PROPERTY(float radius
               READ radius WRITE setRadius NOTIFY radiusChanged)

    Q_PROPERTY(float innerRadius
               READ innerRadius WRITE setInnerRadius NOTIFY innerRadiusChanged)

    Q_PROPERTY(float startAngle
               READ startAngle WRITE setStartAngle NOTIFY startAngleChanged)

    Q_PROPERTY(float sweepAngle
               READ sweepAngle WRITE setSweepAngle NOTIFY sweepAngleChanged)

    Q_PROPERTY(float thickness
               READ thickness WRITE setThickness NOTIFY thicknessChanged)

public:
    explicit XrPieSliceGeometry();

    float radius() const { return m_radius; }
    void setRadius(float value);

    float innerRadius() const { return m_innerRadius; }
    void setInnerRadius(float value);

    float startAngle() const { return m_startAngle; }
    void setStartAngle(float value);

    float sweepAngle() const { return m_sweepAngle; }
    void setSweepAngle(float value);

    float thickness() const { return m_thickness; }
    void setThickness(float value);

Q_SIGNALS:
    void radiusChanged();
    void innerRadiusChanged();
    void startAngleChanged();
    void sweepAngleChanged();
    void thicknessChanged();

private:
    void rebuildGeometry();

    float m_radius = 1.0f;
    float m_innerRadius = 0.0f;
    float m_startAngle = 0.0f;
    float m_sweepAngle = 45.0f;
    float m_thickness = 0.2f;
};
