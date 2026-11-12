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
kconfig_target_kcfg_file(filelightInternal
    FILE filelightsettings.kcfg
    CLASS_NAME FilelightSettings
    MUTATORS
)
if (WIN32)
    target_sources(filelightInternal PRIVATE windowsWalker.cpp windowsWalker.h)
else()
    target_sources(filelightInternal PRIVATE posixWalker.cpp posixWalker.h)
endif()
target_link_libraries(filelightInternal
    Qt6::Gui
    Qt6::Qml
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::I18n
    KF6::KIOWidgets # Only used for KDirLister, may be able to move away from that.
    KF6::Crash
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
    windowThemer.cpp windowThemer.h
)
target_include_directories(filelight PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/radialMap)

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
    PRIVATE
        KF6::I18n
        KF6::I18nQml
        KF6::ColorScheme
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Qml
        filelightInternal
)

ecm_add_qml_module(filelight URI org.kde.filelight
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src/org/kde/filelight)
ecm_finalize_qml_module(filelight DESTINATION ${KDE_INSTALL_QMLDIR})
# Somehow qmlls doesn't find it in bin even though the qmlls.ini tells it to look there :(
add_custom_target(link_bin_org ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink bin/org ${CMAKE_BINARY_DIR}/org)
add_dependencies(filelight link_bin_org)

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
