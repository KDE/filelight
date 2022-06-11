/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <memory>

#include "fileTree.h"
#include <QColor>
#include <QObject>
#include <QQmlEngine>
#include <QUrl>
namespace RadialMap
{
class Segment : public QObject // all angles are in 16ths of degrees
{
    Q_OBJECT
public:
    bool m_highlight = false;
    Q_SIGNAL void highlightChanged();
    Q_PROPERTY(bool fake MEMBER m_fake CONSTANT)

    Q_PROPERTY(QColor color READ color NOTIFY paletteChanged)
    Q_PROPERTY(QColor brush READ brush NOTIFY paletteChanged)
    Q_PROPERTY(QColor pen READ pen NOTIFY paletteChanged)
    Q_SIGNAL void paletteChanged();

    Q_PROPERTY(QString uuid MEMBER m_uuid CONSTANT)

    Segment(const std::shared_ptr<File> &f, uint s, uint l, bool isFake = false);
    ~Segment() override;
    Q_DISABLE_COPY_MOVE(Segment);

    Q_INVOKABLE uint start() const
    {
        return m_angleStart;
    }
    Q_INVOKABLE uint length() const
    {
        return m_angleSegment;
    }
    Q_INVOKABLE uint end() const
    {
        return m_angleStart + m_angleSegment;
    }
    Q_INVOKABLE QUrl url() const
    {
        return m_file->url();
    }
    Q_INVOKABLE bool isFolder() const
    {
        return m_file->isFolder();
    }
    Q_INVOKABLE QString displayName() const
    {
        return m_file->displayName();
    }
    Q_INVOKABLE QString displayPath() const
    {
        return m_file->displayPath();
    }

    const std::shared_ptr<File> file() const
    {
        return m_file;
    }
    Q_INVOKABLE const QColor &pen() const
    {
        return m_pen;
    }
    Q_INVOKABLE const QColor &brush() const
    {
        return m_brush;
    }

    Q_INVOKABLE QString color()
    {
        return m_brush.name();
    }
    bool isFake() const
    {
        return m_fake;
    }
    bool hasHiddenChildren() const
    {
        return m_hasHiddenChildren;
    }

    Q_INVOKABLE QString humanReadableSize() const
    {
        return m_file->humanReadableSize();
    }

    Q_INVOKABLE uint files() const
    {
        if (auto folder = std::dynamic_pointer_cast<Folder>(file())) {
            return folder->children();
        }
        return 0;
    }

    bool intersects(uint a) const
    {
        return ((a >= start()) && (a < end()));
    }

    QString uuid() const
    {
        return m_uuid;
    }

    friend class Map;
    friend class Builder;

private:
    void setPalette(const QColor &p, const QColor &b)
    {
        m_pen = p;
        m_brush = b;
        Q_EMIT paletteChanged();
    }

    const uint m_angleStart;
    const uint m_angleSegment;
    const std::shared_ptr<File> m_file = nullptr;
    QColor m_pen, m_brush;
    bool m_hasHiddenChildren = false;
    const bool m_fake;
    const QString m_uuid;
};
} // namespace RadialMap

#ifndef PI
#define PI 3.141592653589793
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#define MIN_RING_BREADTH 20
#define MAX_RING_BREADTH 60
#define DEFAULT_RING_DEPTH 4 // first level = 0
#define MIN_RING_DEPTH 0

#define MAP_HIDDEN_TRIANGLE_SIZE 5

#define LABEL_MAP_SPACER 7
#define LABEL_TEXT_HMARGIN 5
#define LABEL_TEXT_VMARGIN 0
#define LABEL_ANGLE_MARGIN 32
#define LABEL_MIN_ANGLE_FACTOR 0.05
#define LABEL_MAX_CHARS 30

// QPainter::paintPie:
// The startAngle and spanAngle must be specified in 1/16th of a degree, i.e. a full circle equals 5760 (16 * 360). Positive values for the angles mean
// counter-clockwise while negative values mean the clockwise direction. Zero degrees is at the 3 o'clock position.
#define MAX_DEGREE 5760
#define DEGREE_FACTOR 16
