/***********************************************************************
 * SPDX-FileCopyrightText: 2020 Shubham <aryan100jangid@gmail.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#ifndef TESTFILETREE_H
#define TESTFILETREE_H

#include <memory>

#include "fileTree.h"

#include <QTest>

class TestFileTree : public QObject
{
    Q_OBJECT

public:
    TestFileTree();

private:
    void testFileName();
    void testFileSize();
    void testFilePath();
    void testDuplicate();

private:
    std::unique_ptr<File> fl;
};

#endif
