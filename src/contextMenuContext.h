// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <memory>

#include <QObject>
#include <QQmlEngine>

#include "radialMap.h"

class File;

namespace RadialMap
{
class Segment;
} // namespace RadialMap

namespace Filelight
{
class FileModel;

class ContextMenuContext : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool deleting MEMBER m_deleting WRITE setDeleting NOTIFY deletingChanged)
public:
    using QObject::QObject;

public Q_SLOTS:
    void openTerminal(RadialMap::Segment *segment);
    void openTerminal(const QUrl &url);

    void doNotScan(RadialMap::Segment *segment);
    void doNotScan(const QUrl &url);

    void copyClipboard(RadialMap::Segment *segment);
    void copyClipboard(const QUrl &url);

    void deleteFileFromSegment(RadialMap::Segment *segment);
    void deleteFile(const std::shared_ptr<File> &file);

    void setDeleting(bool status);

Q_SIGNALS:
    void deletingChanged();
    void deleteFileFailed(QString reason);

private:
    bool m_deleting = false;
};

} // namespace Filelight
