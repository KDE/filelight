/***********************************************************************
* SPDX-FileCopyrightText: 2020 Shubham <aryan100jangid@gmail.com>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/ 

#include "testFileTree.h"

TestFileTree::TestFileTree()
{
    fl = new File("./autotests/core/dummy.txt", 20);
}

TestFileTree::~TestFileTree()
{
    delete fl;
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
    const Folder *folder = new Folder("./autotests/core/");
    const QString fpath = fl->displayPath(folder);
    QVERIFY(!fpath.isEmpty());
}

QTEST_MAIN(TestFileTree)
