// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include "windowThemer.h"

#include <QGuiApplication>
#include <QStyleHints>
#include <QWindow>

#if defined(Q_OS_WINDOWS)
#include <dwmapi.h>
#include <windows.h>
#pragma comment(lib, "Dwmapi")
#endif

namespace
{
void themeWindow([[maybe_unused]] QWindow *window)
{
#if defined(Q_OS_WINDOWS)
    // It is a bit unclear if we need to set the attribute conditionally, but from testing on windows 10 at least it seems
    // necessary. May be something to revisit once 10 is EOL. If 11 automatically switches into light mode we can drop
    // the QStyleHints monitoring. Until then we conditionally switch the attribute false (light) or true (dark/accent).
    const bool supportsDark = qGuiApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    const auto result = DwmSetWindowAttribute(reinterpret_cast<HWND>(window->winId()), DWMWA_USE_IMMERSIVE_DARK_MODE, &supportsDark, sizeof(supportsDark));
    if (result != S_OK) {
        qWarning() << "Failed to set immersive dark mode" << result;
    }
#endif
}
} // namespace

void WindowThemer::setWindow(QWindow *window)
{
    if (window == m_window) {
        return;
    }
    disconnect(m_window);
    m_window = window;
    Q_EMIT windowChanged();

    connect(m_window, &QWindow::visibleChanged, this, &WindowThemer::refresh);
    connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged, this, &WindowThemer::refresh, Qt::UniqueConnection);
    refresh();
}

void WindowThemer::refresh()
{
    if (!m_window->isVisible()) {
        return;
    }

    themeWindow(m_window);
}
