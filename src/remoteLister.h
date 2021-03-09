/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef REMOTELISTER_H
#define REMOTELISTER_H

#include <KDirLister>
#include "fileTree.h"


namespace Filelight
{
class ScanManager;

class RemoteLister : public KDirLister
{
    Q_OBJECT
public:
    RemoteLister(const QUrl &url, QWidget *parent, ScanManager* manager);
    ~RemoteLister() override;

Q_SIGNALS:
    void branchCompleted(Folder* tree);

private Q_SLOTS:
    void onCompleted();
    void onCanceled();

private:
    struct Store *m_root, *m_store;
    ScanManager* m_manager;
};
}

#endif
