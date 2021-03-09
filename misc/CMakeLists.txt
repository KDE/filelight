#######################################################################
# SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#######################################################################

install(PROGRAMS org.kde.filelight.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES filelightrc DESTINATION ${KDE_INSTALL_CONFDIR})
install(FILES filelightui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}//filelight)
install(FILES org.kde.filelight.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

ecm_install_icons(ICONS
    16-apps-filelight.png
    22-apps-filelight.png
    32-apps-filelight.png
    48-apps-filelight.png
    64-apps-filelight.png
    128-apps-filelight.png
    DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
