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

add_library(filelightInternal STATIC
    fileTree.cpp
    directoryIterator.cpp
    fileCleaner.cpp
    scan.cpp
    Config.cpp
    localLister.cpp
    remoteLister.cpp
    fileTree.h
    directoryIterator.h
    fileCleaner.h
    scan.h
    Config.h
    localLister.h
    remoteLister.h
)
ecm_qt_declare_logging_category(filelightInternal
    HEADER filelight_debug.h
    IDENTIFIER FILELIGHT_LOG
    CATEGORY_NAME org.kde.filelight
    DESCRIPTION "filelight"
    EXPORT FILELIGHT
)
if (WIN32)
    target_sources(filelightInternal PRIVATE windowsWalker.cpp)
else()
    target_sources(filelightInternal PRIVATE posixWalker.cpp)
endif()
target_link_libraries(filelightInternal
    Qt6::Gui
    KF6::ConfigCore
    KF6::I18n
    KF6::KIOWidgets # Only used for KDirLister, may be able to move away from that.
)

add_executable(filelight)
target_sources(filelight PRIVATE
    radialMap/map.cpp
    radialMap/radialMap.cpp
    mainContext.cpp
    mainContext.h
    main.cpp
    fileTree.h
    radialMap/map.h
    radialMap/radialMap.h
    radialMap/sincos.h
    scan.h
    define.h
    localLister.h
    remoteLister.h
    Config.h
    qml/qml.qrc
    contextMenuContext.cpp
    contextMenuContext.h
    fileModel.cpp
    fileModel.h
    dropperItem.cpp
    dropperItem.h
    hicolor.qrc
    windowThemer.cpp
)

set(filelight_ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/16-apps-filelight.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/32-apps-filelight.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/48-apps-filelight.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/64-apps-filelight.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc/128-apps-filelight.png
)
ecm_add_app_icon(filelight ICONS
    ${filelight_ICONS})

target_link_libraries(filelight
    KF6::I18n
    KF6::ColorScheme
    Qt6::Quick
    Qt6::QuickControls2
    filelightInternal
)

if(APPLE)
    set_target_properties(filelight PROPERTIES
        MACOSX_BUNDLE_DISPLAY_NAME "Filelight"
        MACOSX_BUNDLE_BUNDLE_NAME "Filelight"
        MACOSX_BUNDLE_LONG_VERSION_STRING "Filelight ${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.filelight"
        MACOSX_BUNDLE_COPYRIGHT "2006-2024 The Filelight Developers")

endif()

install(TARGETS filelight ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
ecm_qt_install_logging_categories(
    EXPORT FILELIGHT
    FILE filelight.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
