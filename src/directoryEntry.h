// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QByteArray>

struct DirectoryEntry {
    QByteArray name;
    bool isSkippable = true;
    bool isDuplicate = false; // Only set once we have counted specific inode
    bool isDir = false;
    bool isFile = false;
    size_t size = 0;
};
