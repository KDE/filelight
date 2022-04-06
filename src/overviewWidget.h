// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QWidget>

class KActionCollection;

namespace Filelight {

class OverviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OverviewWidget(KActionCollection *actionCollection, QWidget *parent);

private:
    void addButtons(KActionCollection *actionCollection);
    void addAction(QAction *action, QLayout *layout);
};

} // namespace Filelight
