/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "radialMap.h"

#include <QUuid>

RadialMap::Segment::Segment(const std::shared_ptr<File> &f, uint s, uint l, bool isFake)
    : m_angleStart(s)
    , m_angleSegment(l)
    , m_file(f)
    , m_fake(isFake)
    , m_uuid(QUuid::createUuid().toString())
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    f->setSegment(m_uuid);
}

RadialMap::Segment::~Segment()
{
    if (m_file->segment() == m_uuid) {
        m_file->setSegment({});
    }
}
