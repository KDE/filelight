// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include <QDebug>
#include <QFileInfo>
#include <QTest>

#include "directoryIterator.h"
#include "test-config.h"

class DirectoryIteratorTest : public QObject
{
    Q_OBJECT
    const QString m_tree = QFINDTESTDATA("iterator-tree");
private Q_SLOTS:
    void testIterate()
    {
#if defined(ITERATOR_TREE_WITH_SYMLINK)
        const auto withSymlink = true;
#else
        const auto withSymlink = false;
#endif
#if defined(ITERATOR_TREE_WITH_LINK)
        const auto withLink = true;
#else
        const auto withLink = false;
#endif

        QMap<QByteArray, DirectoryEntry> entries;
        for (const auto &entry : DirectoryIterator(m_tree.toUtf8())) {
            qDebug() << entry.name;
            QVERIFY(!entries.contains(entry.name));
            entries.insert(entry.name, entry);
        }
        qDebug() << entries.keys();

        auto expectedEntries = 3;
        if (withSymlink) {
            ++expectedEntries;
        }
        if (withLink) {
            ++expectedEntries;
        }
        QCOMPARE(entries.size(), expectedEntries);

        QVERIFY(entries.contains(QByteArrayLiteral("foo")));

        QVERIFY(entries.contains(QByteArrayLiteral("Con 자백")));
        const auto dir = entries[QByteArrayLiteral("Con 자백")];
        QVERIFY(dir.isDir);
        QVERIFY(!dir.isFile);
        QVERIFY(!dir.isSkipable);
        // size doesn't matter, it's ignored

        QVERIFY(entries.contains(QByteArrayLiteral("bar")));
        const auto file = entries[QByteArrayLiteral("bar")];
        QVERIFY(!file.isDir);
        QVERIFY(file.isFile);
        QVERIFY(!file.isSkipable);
#ifdef Q_OS_WINDOWS
        QCOMPARE(file.size, 7682);
#elif defined(Q_OS_FREEBSD)
        QCOMPARE(file.size, 1 * S_BLKSIZE);
#else
        QCOMPARE(file.size, 16 * S_BLKSIZE);
#endif

        if (withSymlink) {
            QVERIFY(entries.contains(QByteArrayLiteral("symlink")));
            const auto symlink = entries[QByteArrayLiteral("symlink")];
            QVERIFY(!symlink.isDir);
            QVERIFY(!symlink.isFile);
            QVERIFY(symlink.isSkipable);
            // size of skippables doesn't matter
        }

        if (withLink) {
            QVERIFY(entries.contains(QByteArrayLiteral("link")));
            const auto symlink = entries[QByteArrayLiteral("link")];
            QVERIFY(!symlink.isDir);
            QVERIFY(symlink.isFile);
            QVERIFY(!symlink.isSkipable);
#ifdef Q_OS_WINDOWS
            QCOMPARE(symlink.size, 7682);
#elif defined(Q_OS_FREEBSD)
            QCOMPARE(file.size, 1 * S_BLKSIZE);
#else
            QCOMPARE(symlink.size, 16 * S_BLKSIZE);
#endif
        }
    }

    void testIterateInsideUnicode()
    {
        QByteArray tree = QFINDTESTDATA("iterator-tree/Con 자백").toUtf8();
        QMap<QByteArray, DirectoryEntry> entries;
        for (const auto &entry : DirectoryIterator(tree)) {
            qDebug() << entry.name;
            QVERIFY(!entries.contains(entry.name));
            entries.insert(entry.name, entry);
        }
        qDebug() << entries.keys();
    }


    // During development there were some bugs with iterating C:/, make sure this finishes eventually and has some entries.
    void testCDrive()
    {
        QMap<QByteArray, DirectoryEntry> entries;
        for (const auto &entry : DirectoryIterator(m_tree.toUtf8())) {
            QVERIFY(!entries.contains(entry.name));
            entries.insert(entry.name, entry);
        }
        QVERIFY(entries.size() > 3); // windows, programs, users
    }


    void testBadPath()
    {
        for (const auto &entry : DirectoryIterator(QByteArrayLiteral("/tmp/filelighttest1312312312313123123123"))) {
            Q_UNUSED(entry);
            QVERIFY(false);
        }
    }
};

QTEST_GUILESS_MAIN(DirectoryIteratorTest)

#include "directoryIteratorTest.moc"
