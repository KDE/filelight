/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <QPixmap>
#include <QRectF>
#include <QString>

#include "Config.h"
#include "fileTree.h"
#include "radialMap.h"

namespace Filelight
{
class ContextMenuContext;
}

namespace RadialMap
{
class Segment;

class Map : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool valid READ isValid NOTIFY signatureChanged)
    Q_PROPERTY(QRectF rect MEMBER m_rect NOTIFY rectChanged)
    Q_PROPERTY(QList<QVariant> signature READ signature NOTIFY signatureChanged)
    Q_PROPERTY(QString overallSize READ overallSize NOTIFY signatureChanged)
    Q_PROPERTY(uint numberOfChildren READ numberOfChildren NOTIFY signatureChanged)
    Q_PROPERTY(QUrl rootUrl READ rootUrl NOTIFY signatureChanged)
    Q_PROPERTY(QObject *rootSegment READ rootSegment NOTIFY signatureChanged)
    Q_PROPERTY(QString displayPath READ displayPath NOTIFY signatureChanged)
    friend class Filelight::ContextMenuContext;
    friend class Item;

public:
    static Map *instance()
    {
        static Map map;
        return &map;
    }
    ~Map() override;
    Q_DISABLE_COPY_MOVE(Map)

    bool isValid() const
    {
        return !m_signature.isEmpty() && m_root;
    }

    QString overallSize() const
    {
        return m_root ? m_root->humanReadableSize() : QString();
    }

    uint numberOfChildren() const
    {
        return m_root ? m_root->children() : 0;
    }

    QUrl rootUrl() const;

    void make(const std::shared_ptr<Folder> &tree, bool = false);
    bool resize(const QRectF &);

    bool isNull() const
    {
        return (m_signature.isEmpty());
    }
    Q_INVOKABLE void invalidate();

    qreal height() const
    {
        return m_rect.height();
    }
    qreal width() const
    {
        return m_rect.width();
    }

    Q_INVOKABLE QList<QVariant> signature();
    QList<QList<Segment *>> rawSignature() const
    {
        return m_signature;
    }

    Q_INVOKABLE QString displayPath() const
    {
        return m_root ? m_root->displayPath() : QString();
    }

    std::shared_ptr<Folder> root()
    {
        return m_root;
    }

    QObject *rootSegment() const;

public Q_SLOTS:
    void zoomIn();
    void zoomOut();
    void refresh(Filelight::Dirty filth);
    void createFromCacheObject(RadialMap::Segment *segment);

Q_SIGNALS:
    void rectChanged();
    void signatureChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Map();
    void colorise();
    void setRingBreadth();
    void findVisibleDepth(const std::shared_ptr<Folder> &dir, uint currentDepth = 0);
    bool build(const std::shared_ptr<Folder> &dir, uint depth = 0, uint a_start = 0, uint a_end = MAX_DEGREE);
    void createFromCache(const std::shared_ptr<Folder> &tree);

    QList<QList<Segment *>> m_signature;

    std::shared_ptr<Folder> m_root;
    std::unique_ptr<Segment> m_rootSegment;
    uint m_minSize{};
    QList<FileSize> m_limits;
    QRectF m_rect;
    uint m_visibleDepth; /// visible level depth of system
    int m_ringBreadth;
    uint m_innerRadius; /// radius of inner circle

    uint MAP_2MARGIN;
};
} // namespace RadialMap
