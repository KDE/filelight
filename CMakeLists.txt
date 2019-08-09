#######################################################################
# Copyright 2008-2014  Martin Sandsmark <martin.sandsmark@kde.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#######################################################################


# KDE Application Version, managed by release script
set(KDE_APPLICATIONS_VERSION_MAJOR "19")
set(KDE_APPLICATIONS_VERSION_MINOR "08")
set(KDE_APPLICATIONS_VERSION_MICRO "0")
set(KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")
set(FILELIGHT_VERSION ${KDE_APPLICATIONS_VERSION})
# minimum requirements

cmake_minimum_required (VERSION 3.5 FATAL_ERROR)
# Spectacle project
project(Filelight VERSION ${FILELIGHT_VERSION})

set(PROJECT_VERSION ${KDE_APPLICATIONS_VERSION})
set(KF5_MIN_VERSION "5.46.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

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

find_package(Qt5 5.9 CONFIG REQUIRED Core Widgets)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    XmlGui # For app
    KIO # For part
    I18n
)
find_package(KF5DocTools) # Optional, not needed on Windows for example.

add_definitions(-DTRANSLATION_DOMAIN=\"filelight\")
if (KF5Config_VERSION VERSION_GREATER "5.56.0")
        add_definitions(-DQT_NO_FOREACH)
        MESSAGE(STATUS "compile without foreach")
endif()

include_directories(src)

add_subdirectory(src)
add_subdirectory(misc)
if (KF5DocTools_FOUND)
    add_subdirectory(doc)
endif()

if (ECM_VERSION VERSION_GREATER "5.58.0")
    install(FILES filelight.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
else()
    install(FILES filelight.categories DESTINATION ${KDE_INSTALL_CONFDIR})
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
