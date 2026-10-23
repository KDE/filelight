#include "XrPieSliceGeometry.h"

#include <QVector>
#include <QVector3D>
#include <QByteArray>
#include <QtMath>
#include <QtGlobal>
#include <algorithm>

XrPieSliceGeometry::XrPieSliceGeometry()
{
    rebuildGeometry();
}

void XrPieSliceGeometry::setRadius(float value)
{
    value = qMax(0.001f, value);
    if (qFuzzyCompare(m_radius, value))
        return;

    m_radius = value;
    Q_EMIT radiusChanged();
    rebuildGeometry();
}

void XrPieSliceGeometry::setInnerRadius(float value)
{
    value = qMax(0.0f, value);
    if (qFuzzyCompare(m_innerRadius + 1.0f, value + 1.0f))
        return;

    m_innerRadius = value;
    Q_EMIT innerRadiusChanged();
    rebuildGeometry();
}

void XrPieSliceGeometry::setStartAngle(float value)
{
    if (qFuzzyCompare(m_startAngle, value))
        return;

    m_startAngle = value;
    Q_EMIT startAngleChanged();
    rebuildGeometry();
}

void XrPieSliceGeometry::setSweepAngle(float value)
{
    value = qBound(0.0f, value, 360.0f);
    if (qFuzzyCompare(m_sweepAngle + 1.0f, value + 1.0f))
        return;

    m_sweepAngle = value;
    Q_EMIT sweepAngleChanged();
    rebuildGeometry();
}

void XrPieSliceGeometry::setThickness(float value)
{
    value = qMax(0.001f, value);
    if (qFuzzyCompare(m_thickness, value))
        return;

    m_thickness = value;
    Q_EMIT thicknessChanged();
    rebuildGeometry();
}

void XrPieSliceGeometry::rebuildGeometry()
{
    constexpr int segments = 48;

    const float pi = qAcos(-1.0f);
    const float start = qDegreesToRadians(m_startAngle);
    const float sweep = qDegreesToRadians(m_sweepAngle);
    const float halfHeight = m_thickness * 0.5f;
    const float innerRadius = qBound(0.0f, m_innerRadius,
                                    qMax(0.0f, m_radius - 0.001f));
    const bool hasInnerWall = innerRadius > 0.0f;

    QVector<float> vertices;
    QVector<quint32> indices;

    // Each vertex: position (x, y, z), normal (nx, ny, nz).
    // Vertices are duplicated at hard edges for correct shading.
    auto addVertex = [&](const QVector3D &p,
                         const QVector3D &n) {
        vertices << p.x() << p.y() << p.z()
                 << n.x() << n.y() << n.z();

        indices.append(quint32(vertices.size() / 6 - 1));
    };

    auto addTriangle = [&](const QVector3D &a,
                           const QVector3D &b,
                           const QVector3D &c,
                           const QVector3D &na,
                           const QVector3D &nb,
                           const QVector3D &nc) {
        addVertex(a, na);
        addVertex(b, nb);
        addVertex(c, nc);
    };

    auto addFlatTriangle = [&](const QVector3D &a,
                               const QVector3D &b,
                               const QVector3D &c) {
        QVector3D n = QVector3D::crossProduct(
            b - a, c - a).normalized();

        addTriangle(a, b, c, n, n, n);
    };

    const QVector3D up(0, 1, 0);
    const QVector3D down(0, -1, 0);

    const QVector3D topCenter(0, halfHeight, 0);
    const QVector3D bottomCenter(0, -halfHeight, 0);

    QVector<QVector3D> topArc;
    QVector<QVector3D> bottomArc;
    QVector<QVector3D> topInnerArc;
    QVector<QVector3D> bottomInnerArc;
    QVector<QVector3D> arcNormals;

    topArc.reserve(segments + 1);
    bottomArc.reserve(segments + 1);
    topInnerArc.reserve(segments + 1);
    bottomInnerArc.reserve(segments + 1);
    arcNormals.reserve(segments + 1);

    // Generate the arc, including both endpoints.
    for (int i = 0; i <= segments; ++i) {
        const float t = float(i) / float(segments);
        const float angle = start + sweep * t;

        const float x = m_radius * qSin(angle);
        const float z = m_radius * qCos(angle);

        topArc.append(QVector3D(x, halfHeight, z));
        bottomArc.append(QVector3D(x, -halfHeight, z));
        topInnerArc.append(QVector3D(innerRadius * qSin(angle),
                                     halfHeight,
                                     innerRadius * qCos(angle)));
        bottomInnerArc.append(QVector3D(innerRadius * qSin(angle),
                                        -halfHeight,
                                        innerRadius * qCos(angle)));

        // Smooth, outward-facing normals on the curved wall.
        arcNormals.append(QVector3D(
            qSin(angle), 0, qCos(angle)));
    }

    if (hasInnerWall) {
        // 1. Annular top and bottom faces.
        for (int i = 0; i < segments; ++i) {
            addTriangle(topInnerArc[i], topArc[i], topArc[i + 1],
                        up, up, up);
            addTriangle(topInnerArc[i], topArc[i + 1], topInnerArc[i + 1],
                        up, up, up);

            addTriangle(bottomInnerArc[i], bottomArc[i + 1], bottomArc[i],
                        down, down, down);
            addTriangle(bottomInnerArc[i], bottomInnerArc[i + 1], bottomArc[i + 1],
                        down, down, down);
        }
    } else {
        // 1. Solid-slice top and bottom faces.
        for (int i = 0; i < segments; ++i) {
            addTriangle(topCenter, topArc[i], topArc[i + 1], up, up, up);
            addTriangle(bottomCenter, bottomArc[i + 1], bottomArc[i],
                        down, down, down);
        }
    }

    // 3. Curved outer wall.
    for (int i = 0; i < segments; ++i) {
        const auto &p0 = topArc[i];
        const auto &p1 = bottomArc[i];
        const auto &p2 = bottomArc[i + 1];
        const auto &p3 = topArc[i + 1];

        const auto &n0 = arcNormals[i];
        const auto &n1 = arcNormals[i + 1];

        addTriangle(p0, p1, p2,
                    n0, n0, n1);

        addTriangle(p0, p2, p3,
                    n0, n1, n1);
    }

    // 4. Curved inner wall. Its exterior faces into the hole.
    if (hasInnerWall) {
        for (int i = 0; i < segments; ++i) {
            const QVector3D n0 = -arcNormals[i];
            const QVector3D n1 = -arcNormals[i + 1];

            addTriangle(topInnerArc[i], bottomInnerArc[i + 1], bottomInnerArc[i],
                        n0, n1, n0);
            addTriangle(topInnerArc[i], topInnerArc[i + 1], bottomInnerArc[i + 1],
                        n0, n1, n1);
        }
    }

    // 5. Start and end radial walls.
    // Its outward normal points toward decreasing angle.
    if (m_sweepAngle > 0.0f) {
        const float a = start;
        const QVector3D startNormal(
            -qCos(a), 0, qSin(a));

        const QVector3D &startTop = hasInnerWall ? topInnerArc[0] : topCenter;
        const QVector3D &startBottom = hasInnerWall ? bottomInnerArc[0] : bottomCenter;
        addTriangle(startTop,
                    startBottom,
                    bottomArc[0],
                    startNormal,
                    startNormal,
                    startNormal);

        addTriangle(startTop,
                    bottomArc[0],
                    topArc[0],
                    startNormal,
                    startNormal,
                    startNormal);

        // Its outward normal points toward increasing angle.
        const float b = start + sweep;
        const QVector3D endNormal(
            qCos(b), 0, -qSin(b));

        const QVector3D &endTop = hasInnerWall ? topInnerArc[segments] : topCenter;
        const QVector3D &endBottom = hasInnerWall ? bottomInnerArc[segments] : bottomCenter;
        addTriangle(endTop,
                    topArc[segments],
                    bottomArc[segments],
                    endNormal,
                    endNormal,
                    endNormal);

        addTriangle(endTop,
                    bottomArc[segments],
                    endBottom,
                    endNormal,
                    endNormal,
                    endNormal);
    }

    // Upload the mesh to Qt Quick 3D.
    QByteArray vertexData(
        reinterpret_cast<const char *>(vertices.constData()),
        vertices.size() * qsizetype(sizeof(float)));

    QByteArray indexData(
        reinterpret_cast<const char *>(indices.constData()),
        indices.size() * qsizetype(sizeof(quint32)));

    clear();

    setStride(6 * sizeof(float));

    addAttribute(Attribute::PositionSemantic,
                 0,
                 Attribute::F32Type);

    addAttribute(Attribute::NormalSemantic,
                 3 * sizeof(float),
                 Attribute::F32Type);

    addAttribute(Attribute::IndexSemantic,
                 0,
                 Attribute::U32Type);

    setVertexData(vertexData);
    setIndexData(indexData);

    setPrimitiveType(PrimitiveType::Triangles);

    setBounds(
        QVector3D(-m_radius, -halfHeight, -m_radius),
        QVector3D( m_radius,  halfHeight,  m_radius));

    update();
}
