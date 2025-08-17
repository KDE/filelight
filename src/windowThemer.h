// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QWindow>

// Sets theme properties for a window. Notably used to opt into dark mode support on windows.
class WindowThemer : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(WindowThemer)
    Q_PROPERTY(QWindow *window MEMBER m_window WRITE setWindow NOTIFY windowChanged REQUIRED)
public:
    using QObject::QObject;
    void setWindow(QWindow *window);

private Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void windowChanged();

private:
    QWindow *m_window = nullptr;
};
