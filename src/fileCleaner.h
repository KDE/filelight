// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include "fileTree.h"

// Tiny helper class to defer file deletion into a thread. The Files are being held until actual cleanup.
class FileCleaner : public QObject
{
public:
    static FileCleaner *instance();
    void clean(const QList<std::shared_ptr<File>> &files);

private:
    using QObject::QObject;
    Q_INVOKABLE void cleanUp(QList<std::shared_ptr<File>> files);
};
