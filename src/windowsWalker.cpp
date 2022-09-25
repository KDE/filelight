// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "windowsWalker.h"
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
            } else {
                qWarning() << m_path << ':' << errorCode << GetLastErrorAsString(errorCode);
            }
        } else {
            if (m_fileinfo.cFileName == std::wstring(L".") || m_fileinfo.cFileName == std::wstring(L"..")) {
                continue;
            }
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

    // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesexw
    WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
    BOOL result = GetFileAttributesExW(new_path.c_str(), GetFileExInfoStandard, (LPVOID)&fileAttributeData);
    if (result == 0) {
        qWarning() << "failed to get attributes for" << QString::fromStdWString(new_path);
        return;
    }

    const auto attributes = fileAttributeData.dwFileAttributes;
    // Reparse points are symlinks or NTFS junctions.
    m_entry.isSkipable = attributes & FILE_ATTRIBUTE_REPARSE_POINT || attributes & FILE_ATTRIBUTE_TEMPORARY;
    m_entry.isDir = attributes & FILE_ATTRIBUTE_DIRECTORY;
    m_entry.isFile = !m_entry.isSkipable && !m_entry.isDir; // fileness is implicit in win32 api
    ULARGE_INTEGER ulargeInt;
    ulargeInt.HighPart = fileAttributeData.nFileSizeHigh;
    ulargeInt.LowPart = fileAttributeData.nFileSizeLow;
    m_entry.size = ulargeInt.QuadPart;
}
