// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import Qt.labs.platform 1.1 as Platform
import org.kde.kirigami 2.19 as Kirigami

Platform.MenuBar {
    Platform.Menu {
        title: i18nc("@item:inmenu", "Scan")

        ActionMenuItem { action: scanFolderAction }
        ActionMenuItem { action: Kirigami.Action { separator: true } }
        ActionMenuItem { action: scanHomeAction }
        ActionMenuItem { action: scanRootAction }
        ActionMenuItem { action: Kirigami.Action { separator: true } }
        ActionMenuItem { action: quitAction }
    }

    Platform.Menu {
        title: i18nc("@item:inmenu", "Settings")

        ActionMenuItem { action: configureAction }
    }

    Platform.Menu {
        title: i18nc("@item:inmenu", "Help")

        ActionMenuItem { action: helpAction }
        ActionMenuItem { action: aboutAction }
    }
}
