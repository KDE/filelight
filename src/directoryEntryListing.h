// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022-2026 Harald Sitter <sitter@kde.org>

#pragma once

#include "directoryEntry.h"
#include <generator>

#ifdef Q_OS_WINDOWS
#include "windowsWalker.h"
using Walker = WindowsWalker;
#else
#include "posixWalker.h"
using Walker = POSIXWalker;
#endif

struct DirectoryEntryListing {
    QByteArray path;

    [[nodiscard]] std::generator<const DirectoryEntry &> generate() const
    {
        Walker walker(path);
        while (true) {
            // Already has the first entry after creation, call .next() at the end!
            if (!walker.m_entry.has_value()) { // EOF
                break;
            }
            co_yield *walker.m_entry;
            walker.next();
        }
    }
};
