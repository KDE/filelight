// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <dirent.h>
#include <sys/stat.h>

#include "directoryEntry.h"

class POSIXWalker
{
public:
    explicit POSIXWalker(const QByteArray &path);
    ~POSIXWalker();
    Q_DISABLE_COPY_MOVE(POSIXWalker) // we hold a pointer, disable sharing

    void next();

    QByteArray m_path;
    DirectoryEntry m_entry;

private:
    void close();

    DIR *m_dir = nullptr;
    int m_dirfd = -1;

    struct stat statbuf{};
};
