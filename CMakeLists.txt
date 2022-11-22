#######################################################################
# SPDX-FileCopyrightText: 2008-2014 Martin Sandsmark <martin.sandsmark@kde.org>
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#######################################################################

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "22")
set(RELEASE_SERVICE_VERSION_MINOR "11")
set(RELEASE_SERVICE_VERSION_MICRO "90")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
set(FILELIGHT_VERSION ${RELEASE_SERVICE_VERSION})
# minimum requirements

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
project(Filelight VERSION ${FILELIGHT_VERSION})

set(PROJECT_VERSION ${RELEASE_SERVICE_VERSION})
set(KF5_MIN_VERSION "5.98.0")
set(QT_REQUIRED_VERSION "5.15.2")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(ECMAddAppIcon)
include(ECMGenerateHeaders)
include(ECMInstallIcons)
include(ECMMarkNonGuiExecutable)
include(ECMOptionalAddSubdirectory)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMFindQmlModule)

find_package(Qt${QT_MAJOR_VERSION} ${QT_REQUIRED_VERSION} CONFIG REQUIRED Core Widgets Svg Quick QuickControls2)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    XmlGui # For app
    KIO # For part
    I18n
)
find_package(KF5DocTools) # Optional, not needed on Windows for example.

find_package(KF5QQC2DesktopStyle ${KF5_MIN_VERSION})
set_package_properties(KF5QQC2DesktopStyle PROPERTIES TYPE RUNTIME)

ecm_find_qmlmodule(org.kde.kirigami 2.19)
ecm_find_qmlmodule(org.kde.quickcharts 1.0)

add_definitions(-DTRANSLATION_DOMAIN=\"filelight\")

ecm_set_disabled_deprecation_versions(QT 5.15.2
    KF 5.92
)

include_directories(src)

add_subdirectory(src)
add_subdirectory(misc)
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

if (KF5DocTools_FOUND)
    add_subdirectory(doc)
endif()

ki18n_install(po)
if (KF5DocTools_FOUND)
    kdoctools_install(po)
endif()

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
