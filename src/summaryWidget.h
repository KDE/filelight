/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef SUMMARYWIDGET_H
#define SUMMARYWIDGET_H

#include "Config.h" // dirty

#include <QUrl>
#include <QWidget>

class Folder;

namespace Filelight {

class SummaryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SummaryWidget(QWidget *parent);
    ~SummaryWidget();

Q_SIGNALS:
    void activated(const QUrl&);
    void canvasDirtied(const Dirty dirt);

private:
    void createDiskMaps();

    QList<Folder*> m_disksFolders;
};

}

#endif
