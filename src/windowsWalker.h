// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <windows.h>

#include <QByteArray>

#include "directoryEntry.h"

class WindowsWalker
{
public:
    explicit WindowsWalker(const QByteArray &path);
    ~WindowsWalker();

    void next();

    HANDLE m_handle = INVALID_HANDLE_VALUE;
    const QByteArray m_path;
    DirectoryEntry m_entry{};
    WIN32_FIND_DATAW m_fileinfo{};

private:
    // Returns the last Win32 error, in string format. Returns an empty string if there is no error.
    QString GetLastErrorAsString(DWORD error);
    void updateEntry();
    std::wstring makeLongPath(const std::wstring &path);

    const std::wstring m_pathW;
    bool m_isLongPath;
    bool m_isUNC;

    Q_DISABLE_COPY_MOVE(WindowsWalker); // we hold a pointer, disable sharing
};
