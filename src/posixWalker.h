// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <dirent.h>
#include <fcntl.h>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>

#include "directoryEntry.h"

class POSIXWalker
{
public:
    explicit POSIXWalker(const QByteArray &path);
    ~POSIXWalker();

    void next();

    QByteArray m_path;
    DirectoryEntry m_entry;

private:
    void close();

    DIR *m_dir = nullptr;
    int m_dirfd = -1;
    // Hard link files we have already counted, so we will ignore them
    std::set<ino_t> m_countedHardlinks;

    struct stat statbuf{};
    Q_DISABLE_COPY_MOVE(POSIXWalker) // we hold a pointer, disable sharing
};
