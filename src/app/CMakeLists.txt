#######################################################################
# SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#######################################################################

set(filelight_SRCS historyAction.cpp mainWindow.cpp main.cpp)

add_executable(filelight ${filelight_SRCS})

target_link_libraries(filelight
    KF5::XmlGui
)

install(TARGETS filelight ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
