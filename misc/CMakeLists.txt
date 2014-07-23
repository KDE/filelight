#######################################################################
# Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
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

install(FILES filelightpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(PROGRAMS filelight.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES filelightrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES filelightpartui.rc DESTINATION ${DATA_INSTALL_DIR}/filelightpart)
install(FILES filelightui.rc DESTINATION ${DATA_INSTALL_DIR}/filelight)

ecm_install_icons(ICONS
    hi32-action-views_filelight.png
    hi16-apps-filelight.png 
    hi22-apps-filelight.png
    hi32-apps-filelight.png
    hi48-apps-filelight.png
    hi64-apps-filelight.png
    DESTINATION ${ICON_INSTALL_DIR})
