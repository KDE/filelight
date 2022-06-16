/***********************************************************************
 * SPDX-FileCopyrightText: 2020 Shubham <aryan100jangid@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "testFileTree.h"

TestFileTree::TestFileTree()
    : fl(std::make_unique<File>("./autotests/core/dummy.txt", 20))
{
}

void TestFileTree::testFileName()
{
    const QString fname = fl->displayName();
    QCOMPARE(QStringLiteral("./autotests/core/dummy.txt"), fname);
}

void TestFileTree::testFileSize()
{
    const quint64 fsize = fl->size();
    QVERIFY(fsize > 0);
}

void TestFileTree::testFilePath()
{
    auto folder = std::make_shared<Folder>("./autotests/core/");
    const QString fpath = fl->displayPath(folder);
    QVERIFY(!fpath.isEmpty());
}

QTEST_MAIN(TestFileTree)
