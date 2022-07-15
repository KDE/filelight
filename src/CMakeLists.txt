#######################################################################
# SPDX-FileCopyrightText: 2008-2014 Martin Sandsmark <martin.sandsmark@kde.org>
# SPDX-FileCopyrightText: 2017-2022 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#######################################################################

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX FILELIGHT
    VERSION_HEADER version.h)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(sincos "math.h" HAVE_SINCOS)
if (HAVE_SINCOS)
    add_definitions(-DHAVE_SINCOS)
endif()

add_library(filelightInternal STATIC fileTree.cpp directoryIterator.cpp)
if (WIN32)
    target_sources(filelightInternal PRIVATE windowsWalker.cpp)
else()
    target_sources(filelightInternal PRIVATE posixWalker.cpp)
endif()
target_link_libraries(filelightInternal
    Qt${QT_MAJOR_VERSION}::Core
    KF5::CoreAddons
)

add_executable(filelight)
target_sources(filelight PRIVATE
    radialMap/item.cpp
    radialMap/map.cpp
    radialMap/radialMap.cpp
    scan.cpp
    Config.cpp
    settingsDialog.cpp
    localLister.cpp
    remoteLister.cpp
    historyAction.cpp
    mainContext.cpp
    main.cpp
    settingsDialog.h
    fileTree.h
    radialMap/map.h
    radialMap/radialMap.h
    radialMap/sincos.h
    scan.h
    define.h
    localLister.h
    remoteLister.h
    historyAction.h
    Config.h
    qml/qml.qrc
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
    KF5::Declarative
    Qt${QT_MAJOR_VERSION}::Svg
    Qt${QT_MAJOR_VERSION}::Quick
    Qt${QT_MAJOR_VERSION}::QuickControls2
    filelightInternal
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

