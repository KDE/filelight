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
    const float effectiveInnerRadius = qBound(0.0f, m_requestedInnerRadius, qMax(0.0f, m_radius - 0.001f));
    if (!qFuzzyCompare(m_innerRadius, effectiveInnerRadius)) {
        m_innerRadius = effectiveInnerRadius;
        Q_EMIT innerRadiusChanged();
    }
    rebuildGeometry();
}

void XrPieSliceGeometry::setInnerRadius(float value)
{
    value = qMax(0.0f, value);
    if (qFuzzyCompare(m_requestedInnerRadius, value))
        return;

    m_requestedInnerRadius = value;
    const float effectiveInnerRadius = qBound(0.0f, m_requestedInnerRadius, qMax(0.0f, m_radius - 0.001f));
    if (!qFuzzyCompare(m_innerRadius, effectiveInnerRadius)) {
        m_innerRadius = effectiveInnerRadius;
        Q_EMIT innerRadiusChanged();
    }
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

void XrPieSliceGeometry::setEdgeOnly(bool value)
{
    if (m_edgeOnly == value)
        return;
    m_edgeOnly = value;
    Q_EMIT edgeOnlyChanged();
    rebuildGeometry();
}

void XrPieSliceGeometry::rebuildGeometry()
{
    constexpr int segments = 48;

    const float pi = qAcos(-1.0f);
    const float start = qDegreesToRadians(m_startAngle);
    const float sweep = qDegreesToRadians(m_sweepAngle);
    const float halfHeight = m_thickness * 0.5f;
    const bool hasInnerWall = m_innerRadius > 0.0f;
    const float bevel = qMin(0.65f, qMin(halfHeight * 0.25f, (m_radius - m_innerRadius) * 0.25f));

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

    const QVector3D up(0, 1, 0);
    const QVector3D down(0, -1, 0);

    const QVector3D topCenter(0, halfHeight, 0);
    const QVector3D bottomCenter(0, -halfHeight, 0);

    if (m_edgeOnly) {
        // Thin inset strips, rather than an enlarged silhouette, keep the
        // highlight on the actual top edge as the camera moves.
        const float width = qMin(0.3f, (m_radius - m_innerRadius) * 0.25f);
        const float y = halfHeight + 0.02f;
        auto point = [y](float radius, float angle) {
            return QVector3D(radius * qSin(angle), y, radius * qCos(angle));
        };
        auto quad = [&](const QVector3D &a, const QVector3D &b, const QVector3D &c, const QVector3D &d) {
            addTriangle(a, b, c, up, up, up);
            addTriangle(a, c, d, up, up, up);
        };
        for (int i = 0; i < segments; ++i) {
            const float a = start + sweep * float(i) / segments;
            const float b = start + sweep * float(i + 1) / segments;
            quad(point(m_radius - bevel - width, a), point(m_radius - bevel, a), point(m_radius - bevel, b), point(m_radius - bevel - width, b));
        }
        if (m_sweepAngle > 0.0f && m_sweepAngle < 360.0f) {
            // Offset toward the interior of the wedge at each radial edge.
            const QVector3D startOffset(qCos(start) * width, 0, -qSin(start) * width);
            const float end = start + sweep;
            const QVector3D endOffset(-qCos(end) * width, 0, qSin(end) * width);
            const QVector3D s0 = point(m_innerRadius, start);
            const QVector3D s1 = point(m_radius - bevel, start);
            const QVector3D e0 = point(m_innerRadius, end);
            const QVector3D e1 = point(m_radius - bevel, end);
            quad(s0, s1, s1 + startOffset, s0 + startOffset);
            quad(e0, e0 + endOffset, e1 + endOffset, e1);
        }
    } else {
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

            const float x = (m_radius - bevel) * qSin(angle);
            const float z = (m_radius - bevel) * qCos(angle);

            topArc.append(QVector3D(x, halfHeight, z));
            bottomArc.append(QVector3D(x, -halfHeight, z));
            topInnerArc.append(QVector3D(m_innerRadius * qSin(angle), halfHeight, m_innerRadius * qCos(angle)));
            bottomInnerArc.append(QVector3D(m_innerRadius * qSin(angle), -halfHeight, m_innerRadius * qCos(angle)));

            // Smooth, outward-facing normals on the curved wall.
            arcNormals.append(QVector3D(qSin(angle), 0, qCos(angle)));
        }

        if (hasInnerWall) {
            // 1. Annular top and bottom faces.
            for (int i = 0; i < segments; ++i) {
                addTriangle(topInnerArc[i], topArc[i], topArc[i + 1], up, up, up);
                addTriangle(topInnerArc[i], topArc[i + 1], topInnerArc[i + 1], up, up, up);

                addTriangle(bottomInnerArc[i], bottomArc[i + 1], bottomArc[i], down, down, down);
                addTriangle(bottomInnerArc[i], bottomInnerArc[i + 1], bottomArc[i + 1], down, down, down);
            }
        } else {
            // 1. Solid-slice top and bottom faces.
            for (int i = 0; i < segments; ++i) {
                addTriangle(topCenter, topArc[i], topArc[i + 1], up, up, up);
                addTriangle(bottomCenter, bottomArc[i + 1], bottomArc[i], down, down, down);
            }
        }

        // The outer wall meets the flat faces through a small, rounded bevel.
        // Interpolated normals let the light pick out the rounded rim.
        for (int i = 0; i < segments; ++i) {
            const auto &n0 = arcNormals[i];
            const auto &n1 = arcNormals[i + 1];
            auto point = [&](const QVector3D &radial, float r, float y) {
                return radial * r + QVector3D(0, y, 0);
            };
            auto band = [&](float r0, float y0, float r1, float y1, float radial0, float vertical0, float radial1, float vertical1) {
                const QVector3D na0 = n0 * radial0 + up * vertical0;
                const QVector3D nb0 = n1 * radial0 + up * vertical0;
                const QVector3D na1 = n0 * radial1 + up * vertical1;
                const QVector3D nb1 = n1 * radial1 + up * vertical1;
                addTriangle(point(n0, r0, y0), point(n0, r1, y1), point(n1, r1, y1), na0, na1, nb1);
                addTriangle(point(n0, r0, y0), point(n1, r1, y1), point(n1, r0, y0), na0, nb1, nb0);
            };

            constexpr int bevelSegments = 3;
            for (int j = 0; j < bevelSegments; ++j) {
                const float a = pi * 0.5f * j / bevelSegments;
                const float b = pi * 0.5f * (j + 1) / bevelSegments;
                band(m_radius - bevel + bevel * qSin(a),
                     halfHeight - bevel + bevel * qCos(a),
                     m_radius - bevel + bevel * qSin(b),
                     halfHeight - bevel + bevel * qCos(b),
                     qSin(a),
                     qCos(a),
                     qSin(b),
                     qCos(b));
            }
            band(m_radius, halfHeight - bevel, m_radius, -halfHeight + bevel, 1, 0, 1, 0);
            for (int j = 0; j < bevelSegments; ++j) {
                const float a = pi * 0.5f * j / bevelSegments;
                const float b = pi * 0.5f * (j + 1) / bevelSegments;
                band(m_radius - bevel + bevel * qCos(a),
                     -halfHeight + bevel - bevel * qSin(a),
                     m_radius - bevel + bevel * qCos(b),
                     -halfHeight + bevel - bevel * qSin(b),
                     qCos(a),
                     -qSin(a),
                     qCos(b),
                     -qSin(b));
            }
        }

        // 4. Curved inner wall. Its exterior faces into the hole.
        if (hasInnerWall) {
            for (int i = 0; i < segments; ++i) {
                const QVector3D n0 = -arcNormals[i];
                const QVector3D n1 = -arcNormals[i + 1];

                addTriangle(topInnerArc[i], bottomInnerArc[i + 1], bottomInnerArc[i], n0, n1, n0);
                addTriangle(topInnerArc[i], topInnerArc[i + 1], bottomInnerArc[i + 1], n0, n1, n1);
            }
        }

        // 5. Start and end radial walls.
        // Its outward normal points toward decreasing angle.
        if (m_sweepAngle > 0.0f) {
            const float a = start;
            const QVector3D startNormal(-qCos(a), 0, qSin(a));

            const QVector3D &startTop = hasInnerWall ? topInnerArc[0] : topCenter;
            const QVector3D &startBottom = hasInnerWall ? bottomInnerArc[0] : bottomCenter;
            addTriangle(startTop, startBottom, bottomArc[0], startNormal, startNormal, startNormal);

            addTriangle(startTop, bottomArc[0], topArc[0], startNormal, startNormal, startNormal);

            const QVector3D startWallTop = arcNormals[0] * m_radius + up * (halfHeight - bevel);
            const QVector3D startWallBottom = arcNormals[0] * m_radius - up * (halfHeight - bevel);
            addTriangle(topArc[0], bottomArc[0], startWallBottom, startNormal, startNormal, startNormal);
            addTriangle(topArc[0], startWallBottom, startWallTop, startNormal, startNormal, startNormal);

            // Its outward normal points toward increasing angle.
            const float b = start + sweep;
            const QVector3D endNormal(qCos(b), 0, -qSin(b));

            const QVector3D &endTop = hasInnerWall ? topInnerArc[segments] : topCenter;
            const QVector3D &endBottom = hasInnerWall ? bottomInnerArc[segments] : bottomCenter;
            addTriangle(endTop, topArc[segments], bottomArc[segments], endNormal, endNormal, endNormal);

            addTriangle(endTop, bottomArc[segments], endBottom, endNormal, endNormal, endNormal);

            const QVector3D endWallTop = arcNormals[segments] * m_radius + up * (halfHeight - bevel);
            const QVector3D endWallBottom = arcNormals[segments] * m_radius - up * (halfHeight - bevel);
            addTriangle(topArc[segments], endWallTop, endWallBottom, endNormal, endNormal, endNormal);
            addTriangle(topArc[segments], endWallBottom, bottomArc[segments], endNormal, endNormal, endNormal);
        }
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

    setBounds(QVector3D(-m_radius, -halfHeight, -m_radius), QVector3D(m_radius, halfHeight + (m_edgeOnly ? 0.02f : 0.0f), m_radius));

    update();
}
