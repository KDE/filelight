/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <memory>

#include <KDirLister>

#include "fileTree.h"

namespace Filelight
{
class ScanManager;
struct Store;

class RemoteLister : public KDirLister
{
    Q_OBJECT
public:
    RemoteLister(const QUrl &url, ScanManager *parent);

Q_SIGNALS:
    void branchCompleted(Folder *tree);

private Q_SLOTS:
    void onCompleted();
    void onCanceled();

private:
    std::shared_ptr<Store> m_root;
    std::shared_ptr<Store> m_store;
    ScanManager *m_manager;
};
} // namespace Filelight
