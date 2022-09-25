// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include "directoryEntry.h"
#include <memory>

#ifdef Q_OS_WINDOWS
#include "windowsWalker.h"
template<class T = WindowsWalker>
#else
#include "posixWalker.h"
template<class T = POSIXWalker>
#endif
class DirectoryIterator
{
public:
    using value_type = DirectoryEntry;
    using difference_type = std::ptrdiff_t;
    using pointer = const DirectoryEntry *;
    using reference = const DirectoryEntry &;
    using iterator_category = std::input_iterator_tag;

    DirectoryIterator() = default;
    explicit DirectoryIterator(const QByteArray &path)
        : m_walker(std::make_shared<T>(path))
    {
    }

    const DirectoryEntry &operator*() const
    {
        return m_walker->m_entry;
    };

    DirectoryIterator &operator++()
    {
        m_walker->next();
        return *this;
    }

    std::shared_ptr<T> m_walker = std::make_shared<T>(QByteArray()); // always have a valid walker to simplify accessing into it
private:
    friend bool operator==(const DirectoryIterator &lhs, const DirectoryIterator &rhs) noexcept
    {
        return lhs.m_walker->m_entry.name == rhs.m_walker->m_entry.name;
    }

    friend bool operator!=(const DirectoryIterator &lhs, const DirectoryIterator &rhs) noexcept
    {
        return !(lhs == rhs);
    }
};

template<class T>
DirectoryIterator<T> begin(DirectoryIterator<T> iter) noexcept
{
    return DirectoryIterator<T>(iter.m_walker->m_path);
}

template<class T>
DirectoryIterator<T> end(DirectoryIterator<T> iter) noexcept
{
    Q_UNUSED(iter);
    return {};
}
