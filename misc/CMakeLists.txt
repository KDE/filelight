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

install(PROGRAMS org.kde.filelight.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES filelightrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES filelightui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}//filelight)

ecm_install_icons(ICONS
    32-actions-views_filelight.png
    16-apps-filelight.png
    22-apps-filelight.png
    32-apps-filelight.png
    48-apps-filelight.png
    64-apps-filelight.png
    DESTINATION ${ICON_INSTALL_DIR} THEME hicolor)
