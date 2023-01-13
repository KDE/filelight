// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include <QDebug>
#include <QSignalSpy>
#include <QTest>

#include "fileTree.h"
#include "localLister.h"
#include "scan.h"
#include "test-config.h"

Q_DECLARE_METATYPE(std::shared_ptr<Folder>)

class LocalListerTest : public QObject
{
    Q_OBJECT
    const QString m_tree = QFINDTESTDATA("iterator-tree");
private Q_SLOTS:
    void testRun()
    {
        qRegisterMetaType<std::shared_ptr<File>>("std::shared_ptr<File>");
        qRegisterMetaType<std::shared_ptr<Folder>>("std::shared_ptr<Folder>");

        auto cache = new QList<std::shared_ptr<Folder>>;
        Filelight::ScanManager manager(nullptr);
        Filelight::LocalLister lister(m_tree, cache, &manager);
        QSignalSpy spy(&lister, &Filelight::LocalLister::branchCompleted);
        lister.start();
        lister.wait();
        spy.wait();
        const auto arguments = spy.takeFirst();
        const auto root = arguments.at(0).value<std::shared_ptr<Folder>>();

        QVERIFY(root);
        QCOMPARE(root->children(), 3);
#ifdef Q_OS_LINUX
        QCOMPARE(root->size(), 8192);
#endif
    }
};

QTEST_GUILESS_MAIN(LocalListerTest)

#include "localListerTest.moc"
