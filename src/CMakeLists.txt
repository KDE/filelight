#######################################################################
# SPDX-FileCopyrightText: 2008-2014 Martin Sandsmark <martin.sandsmark@kde.org>
# SPDX-FileCopyrightText: 2017 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#######################################################################

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX FILELIGHT
    VERSION_HEADER version.h)

add_executable(filelight)
target_sources(filelight PRIVATE
    radialMap/widget.cpp
    radialMap/map.cpp
    radialMap/widgetEvents.cpp
    radialMap/labels.cpp
    scan.cpp
    progressBox.cpp
    Config.cpp
    settingsDialog.cpp
    fileTree.cpp
    localLister.cpp
    remoteLister.cpp
    summaryWidget.cpp
    historyAction.cpp
    mainWindow.cpp
    main.cpp
)
ecm_qt_declare_logging_category(filelight HEADER filelight_debug.h IDENTIFIER FILELIGHT_LOG CATEGORY_NAME org.kde.filelight DESCRIPTION "filelight" EXPORT FILELIGHT)

set(filelight_ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/16-apps-filelight.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/32-apps-filelight.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/48-apps-filelight.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/64-apps-filelight.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/128-apps-filelight.png
)
ecm_add_app_icon(filelight ICONS
    ${filelight_ICONS})
ki18n_wrap_ui(filelight dialog.ui)

target_link_libraries(filelight
    KF5::I18n
    KF5::XmlGui
    KF5::KIOWidgets # Only used for KDirLister, may be able to move away from that.
    Qt::Svg
)
if (WIN32)
    find_package(KDEWin REQUIRED)
    target_link_libraries(filelight kdewin)
endif()

install(TARGETS filelight ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
ecm_qt_install_logging_categories(
        EXPORT FILELIGHT
        FILE filelight.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
        )

