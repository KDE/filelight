// SPDX-FileCopyrightText: 2026 Vladimir Ivanov <me@un1or.name>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#include "Config.h"
#include "fileCleaner.h"
#include "radialMap/map.h"

#include <QStandardPaths>
#include <QTest>

class MapUpdateTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        FileCleaner::instance();
        Config::instance()->read();
        Config::instance()->scheme = Filelight::Rainbow;
        Config::instance()->contrast = 75;
        Config::instance()->showSmallFiles = false;
    }

    void publishesCompleteMap_data()
    {
        QTest::addColumn<int>("entries");
        QTest::addColumn<int>("segments");
        QTest::newRow("empty-root") << 0 << 0;
        QTest::newRow("empty-child") << 1 << 0;
        QTest::newRow("grouped-files") << 3 << 2;
    }

    void publishesCompleteMap()
    {
        QFETCH(int, entries);
        QFETCH(int, segments);
        auto map = RadialMap::Map::instance();
        const auto previous = std::make_shared<Folder>("previous/");
        previous->append("file", 100);
        map->resize({0, 0, 600, 600});
        map->make(previous);

        const auto root = std::make_shared<Folder>("root/");
        if (entries == 1) {
            root->append(std::make_shared<Folder>("empty/"));
        } else if (entries == 3) {
            root->append("large", 10000);
            root->append("small-a", 30);
            root->append("small-b", 30);
        }

        QObject observer;
        int publications = 0;
        QObject *publishedRoot = nullptr;
        QList<QColor> publishedColors;
        connect(map, &RadialMap::Map::signatureChanged, &observer, [&] {
            ++publications;
            publishedRoot = map->rootSegment();
            for (const auto &ring : map->rawSignature()) {
                for (const auto segment : ring) {
                    publishedColors.append(segment->brush());
                }
            }
        });
        map->make(root);
        QCOMPARE(publications, 1);
        QCOMPARE(publishedRoot, map->rootSegment());
        QCOMPARE(publishedColors.size(), segments);
        QList<QColor> finalColors;
        for (const auto &ring : map->rawSignature()) {
            for (const auto segment : ring) {
                finalColors.append(segment->brush());
            }
        }
        QCOMPARE(publishedColors, finalColors);
    }
};

QTEST_MAIN(MapUpdateTest)
#include "mapUpdateTest.moc"
