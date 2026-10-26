/***********************************************************************
 * SPDX-FileCopyrightText: 2020 Shubham <aryan100jangid@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#include "testFileTree.h"

TestFileTree::TestFileTree()
    : m_file(std::make_unique<File>(qUtf8Printable(QFINDTESTDATA("dummy.txt")), 20))
{
}

void TestFileTree::testFileName()
{
    const QString fname = m_file->displayName();
    QCOMPARE(QFINDTESTDATA("dummy.txt"), fname);
}

void TestFileTree::testFileSize()
{
    const quint64 fsize = m_file->size();
    QVERIFY(fsize > 0);
}

void TestFileTree::testFilePath()
{
    auto folder = std::make_shared<Folder>("./autotests/core/");
    const QString fpath = m_file->displayPath(folder);
    QVERIFY(!fpath.isEmpty());
}

void TestFileTree::testDuplicate()
{
    auto foo = std::make_shared<Folder>("foo/");
    auto bar = std::make_shared<Folder>("bar/");
    auto file = std::make_shared<Folder>("file/");
    auto light = std::make_shared<Folder>("light/");
    bar->append("onion", 1024);
    light->append("torch", 128);
    file->append(light);
    foo->append(bar);
    foo->append(file);
    foo->append("shallot", 512);
    auto other = foo->duplicate();
    qDebug() << other->size();
    QCOMPARE(other->size(), foo->size());
    QCOMPARE(other->children(), foo->children());
}

void TestFileTree::testRemove()
{
    auto root = std::make_shared<Folder>("root/");
    auto docs = std::make_shared<Folder>("docs/");
    auto nested = std::make_shared<Folder>("nested/");
    nested->append("book", 300);
    docs->append(nested);
    docs->append("note", 100);
    root->append(docs);
    root->append("sibling", 200);
    const auto note = docs->files.last();

    docs->remove(nested);
    QCOMPARE(docs->size(), 100);
    QCOMPARE(docs->children(), 1);
    QCOMPARE(root->size(), 300);
    QCOMPARE(root->children(), 3);
    QCOMPARE(root->files.first()->decodedName(), QStringLiteral("sibling"));
    QVERIFY(!nested->parent());

    docs->remove(nested);
    root->remove(note);
    QCOMPARE(docs->size(), 100);
    QCOMPARE(docs->children(), 1);
    QCOMPARE(root->size(), 300);
    QCOMPARE(root->children(), 3);

    docs->remove(docs->files.first());
    QVERIFY(docs->files.isEmpty());
    QCOMPARE(docs->size(), 0);
    QCOMPARE(docs->children(), 0);
    QCOMPARE(root->size(), 200);
    QCOMPARE(root->children(), 2);
}

QTEST_MAIN(TestFileTree)
