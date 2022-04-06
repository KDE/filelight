// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "overviewWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QToolButton>

#include <KActionCollection>
#include <KIconLoader>
#include <KLocalizedString>

static constexpr auto arbitraryTinySpacing = 6;

Filelight::OverviewWidget::OverviewWidget(KActionCollection *actionCollection, QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setSpacing(arbitraryTinySpacing);
    setLayout(layout);

    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    auto iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("filelight")).pixmap(KIconLoader::SizeEnormous));
    layout->addWidget(iconLabel, 0, Qt::AlignCenter);

    auto welcomeLabel = new QLabel(i18nc("@title", "Welcome to Filelight"), this);
    QFont font = welcomeLabel->font();
    font.setBold(true);
    welcomeLabel->setFont(font);
    layout->addWidget(welcomeLabel, 0, Qt::AlignCenter);
    static constexpr auto arbitraryTinySpacing = 8;
    layout->addSpacerItem(new QSpacerItem(0, arbitraryTinySpacing, QSizePolicy::Fixed));

    addButtons(actionCollection);

    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
}

void Filelight::OverviewWidget::addAction(QAction *action, QLayout *layout)
{
    auto button = new QToolButton(this);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setIconSize(QSize(KIconLoader::SizeHuge, KIconLoader::SizeHuge));
    button->setDefaultAction(action);
    layout->addWidget(button);
}

void Filelight::OverviewWidget::addButtons(KActionCollection *actionCollection)
{
    auto buttonLayout = new QHBoxLayout(this);
    buttonLayout->setSpacing(arbitraryTinySpacing);
    layout()->addItem(buttonLayout);

    buttonLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

    addAction(actionCollection->action(QStringLiteral("scan_folder")), buttonLayout);
    addAction(actionCollection->action(QStringLiteral("scan_home")), buttonLayout);
    static const bool inSandbox =
        !(QStandardPaths::locate(QStandardPaths::RuntimeLocation, QLatin1String("flatpak-info")).isEmpty() ||
          qEnvironmentVariableIsSet("SNAP"));
    if (!inSandbox) {
        addAction(actionCollection->action(QStringLiteral("scan_root")), buttonLayout);
    }

    buttonLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

}
