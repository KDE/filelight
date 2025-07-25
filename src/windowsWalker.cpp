// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "windowsWalker.h"
#include <QDebug>
#include <QScopeGuard>
#include <QString>

WindowsWalker::WindowsWalker(const QByteArray &path)
    : m_path(path)
    , m_pathW(QString::fromUtf8(m_path).toStdWString())
{
    if (path.isEmpty()) {
        return;
    }

    const std::wstring filespec = m_pathW + L"\\*";
    m_handle = FindFirstFileW(filespec.c_str(), &m_fileinfo);
    if (m_handle == INVALID_HANDLE_VALUE) {
        const DWORD errorCode = GetLastError();
        qWarning() << "Failed to open" << path << ':' << errorCode << GetLastErrorAsString(errorCode);
    } else {
        if (m_fileinfo.cFileName == std::wstring(L".") || m_fileinfo.cFileName == std::wstring(L"..")) {
            // stream past . and ..
            next();
        } else {
            updateEntry();
        }
    }
}

WindowsWalker::~WindowsWalker()
{
    close();
}

void WindowsWalker::close()
{
    if (m_handle != INVALID_HANDLE_VALUE) {
        FindClose(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

void WindowsWalker::next()
{
    while (true) {
        m_entry = {}; // reset

        if (FindNextFileW(m_handle, &m_fileinfo) == 0) { // error
            const DWORD errorCode = GetLastError();
            if (errorCode == ERROR_NO_MORE_FILES) {
                // qDebug() << "no more files";
                close();
                return;
            }
            // WARNING: do not access m_fileinfo, it has undefined content!
            qWarning() << m_path << ':' << errorCode << GetLastErrorAsString(errorCode);
            continue;
        }

        if (m_fileinfo.cFileName == std::wstring(L".") || m_fileinfo.cFileName == std::wstring(L"..")) {
            continue;
        }

        updateEntry();
        return;
    }
}

QString WindowsWalker::GetLastErrorAsString(DWORD error)
{
    Q_ASSERT(error != NO_ERROR && error != ERROR_SUCCESS);

    LPWSTR buffer = nullptr;
    auto freeBuffer = qScopeGuard([buffer] {
        LocalFree(buffer);
    });

    size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 nullptr,
                                 error,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 (LPWSTR)&buffer,
                                 0,
                                 nullptr);

    return QString::fromWCharArray(buffer);
}

void WindowsWalker::updateEntry()
{
    m_entry = {}; // reset

    const std::wstring name(m_fileinfo.cFileName);
    m_entry.name = QString::fromStdWString(name).toUtf8();

    auto new_path = m_pathW + L"/" + name;

    // We already have all file information we need. If we ever need more, be very careful with which API you use.
    // Windows file APIs have huge performance differences! Best benchmark all options you can find.

    const auto attributes = m_fileinfo.dwFileAttributes;
    // Reparse points are symlinks or NTFS junctions.
    m_entry.isSkippable = attributes & FILE_ATTRIBUTE_REPARSE_POINT || attributes & FILE_ATTRIBUTE_TEMPORARY;
    m_entry.isDir = attributes & FILE_ATTRIBUTE_DIRECTORY;
    m_entry.isFile = !m_entry.isSkippable && !m_entry.isDir; // fileness is implicit in win32 api
    ULARGE_INTEGER ulargeInt;
    if (attributes & FILE_ATTRIBUTE_COMPRESSED || attributes & FILE_ATTRIBUTE_SPARSE_FILE || attributes & FILE_ATTRIBUTE_UNPINNED) {
        ulargeInt.HighPart = 0;
        ulargeInt.LowPart = GetCompressedFileSizeW(new_path.c_str(), &ulargeInt.HighPart);
        if (GetLastError() != ERROR_SUCCESS && ulargeInt.LowPart == INVALID_FILE_SIZE) {
            ulargeInt.HighPart = m_fileinfo.nFileSizeHigh;
            ulargeInt.LowPart = m_fileinfo.nFileSizeLow;
        }
    } else {
        ulargeInt.HighPart = m_fileinfo.nFileSizeHigh;
        ulargeInt.LowPart = m_fileinfo.nFileSizeLow;
    }

    m_entry.size = ulargeInt.QuadPart;
}
