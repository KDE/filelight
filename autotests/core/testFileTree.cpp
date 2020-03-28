/***********************************************************************
* Copyright 2020 Shubham <aryan100jangid@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
