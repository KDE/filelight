// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick
import Qt.labs.platform 1.1 as Platform
import org.kde.kirigami 2.19 as Kirigami

Platform.MenuBar {
    id: bar
    // On Windows the bar doesn't theme properly plus we only want it on linux anyway for its global menu support.
    readonly property bool visible: Qt.platform.os !== "windows"

    Platform.Menu {
        visible: bar.visible
        title: i18nc("@item:inmenu", "Scan")

        ActionMenuItem { action: scanFolderAction }
        ActionMenuItem { action: Kirigami.Action { separator: true } }
        ActionMenuItem { action: scanHomeAction }
        ActionMenuItem { action: scanRootAction }
        ActionMenuItem { action: Kirigami.Action { separator: true } }
        ActionMenuItem { action: quitAction }
    }

    Platform.Menu {
        visible: bar.visible
        title: i18nc("@item:inmenu", "Settings")

        ActionMenuItem { action: configureAction }
    }

    Platform.Menu {
        visible: bar.visible
        title: i18nc("@item:inmenu", "Help")

        ActionMenuItem { action: helpAction }
        ActionMenuItem { action: aboutAction }
    }
}
