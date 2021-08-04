#######################################################################
# SPDX-FileCopyrightText: 2008-2014 Martin Sandsmark <martin.sandsmark@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#######################################################################


# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "21")
set(RELEASE_SERVICE_VERSION_MINOR "08")
set(RELEASE_SERVICE_VERSION_MICRO "0")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
set(FILELIGHT_VERSION ${RELEASE_SERVICE_VERSION})
# minimum requirements

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
project(Filelight VERSION ${FILELIGHT_VERSION})

set(PROJECT_VERSION ${RELEASE_SERVICE_VERSION})
set(KF5_MIN_VERSION "5.83.0")
set(QT_REQUIRED_VERSION "5.15.0")

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
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Core Widgets Svg)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    XmlGui # For app
    KIO # For part
    I18n
)
find_package(KF5DocTools) # Optional, not needed on Windows for example.

add_definitions(-DTRANSLATION_DOMAIN=\"filelight\")
add_definitions(-DQT_NO_FOREACH)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050f00)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055300)

include_directories(src)

add_subdirectory(src)
add_subdirectory(misc)
add_subdirectory(autotests)

if (KF5DocTools_FOUND)
    add_subdirectory(doc)
endif()

ki18n_install(po)
if (KF5DocTools_FOUND)
    kdoctools_install(po)
endif()
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
