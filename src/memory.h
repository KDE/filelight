// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <memory>

#include <QObject>

// Make a slot-safe QObject shared_ptr. Uses deleteLater() as deleter.
// Mind that this is usually preferred over unique_ptr when dealing with complex usage scenarios because shared_ptr's
// deleter is type-erased and thus can consume a greater number of inputs (e.g. a nullptr_t)
template<typename T, typename... Args>
std::shared_ptr<T> make_shared_qobject(Args &&...args)
{
    T *raw = new T(std::forward<Args>(args)...);
    return std::shared_ptr<T>(raw, [](T *obj) {
        if (obj) {
            obj->deleteLater();
        }
    });
}
