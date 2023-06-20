// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include <QDebug>
#include <QSignalSpy>
#include <QTest>

#include "fileTree.h"
#include "scan.h"

Q_DECLARE_METATYPE(std::shared_ptr<Folder>)

class ScanManagerTest : public QObject
{
    Q_OBJECT
    const QString m_tree = QFINDTESTDATA("iterator-tree");
private Q_SLOTS:
    void testRun()
    {
        qRegisterMetaType<std::shared_ptr<File>>("std::shared_ptr<File>");
        qRegisterMetaType<std::shared_ptr<Folder>>("std::shared_ptr<Folder>");

        Filelight::ScanManager manager(nullptr);

        {
            QSignalSpy spy(&manager, &Filelight::ScanManager::completed);
            manager.start(QUrl(QStringLiteral("file://") + m_tree));
            spy.wait();
        }

        {
            // rescan to make sure the cache doesn't blow up
            QSignalSpy spy(&manager, &Filelight::ScanManager::completed);
            manager.start(QUrl(QStringLiteral("file://") + m_tree));
            spy.wait();
        }

        {
            // invalidate part of the tree and make sure the scanner doesn't blow up
            manager.invalidateCacheFor(QUrl(QStringLiteral("file://") + m_tree + QStringLiteral("/foo")));
            QSignalSpy spy(&manager, &Filelight::ScanManager::completed);
            manager.start(QUrl(QStringLiteral("file://") + m_tree));
            spy.wait();
        }
    }
};

QTEST_MAIN(ScanManagerTest)

#include "scanManagerTest.moc"
