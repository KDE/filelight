/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "summaryWidget.h"

#include "fileTree.h"
#include "radialMap/radialMap.h"
#include "radialMap/widget.h"



#include <KLocalizedString>

#include <QLabel>
#include <QApplication>
#include <QByteArray>
#include <QList>
#include <QMouseEvent>
#include <QLayout>
#include <QStorageInfo>
#include <QMap>

namespace Filelight
{

struct Disk
{
    QString mount;
    QString name;

    qint64 size;
    qint64 used;
    qint64 free; //NOTE used+avail != size (clustersize!)
};


struct DiskList : QMap<QString, Disk>
{
    DiskList();
};


class MyRadialMap : public RadialMap::Widget
{
    Q_OBJECT

public:
    MyRadialMap(QWidget *parent)
            : RadialMap::Widget(parent, true)
    {
    }

    virtual void setCursor(const QCursor &c)
    {
        if (focusSegment() && focusSegment()->file()->decodedName() == QLatin1String( "Used" ))
            RadialMap::Widget::setCursor(c);
        else
            unsetCursor();
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (focusSegment() == rootSegment() && e->button() == Qt::RightButton) {
            // we will allow right clicks to the center circle
            RadialMap::Widget::mousePressEvent(e);
        } else if (e->button() == Qt::LeftButton) {
            // and clicks to the used segment
            Q_EMIT activated(url());
        }
    }
};



SummaryWidget::SummaryWidget(QWidget *parent)
        : QWidget(parent)
{
    qApp->setOverrideCursor(Qt::WaitCursor);
    setLayout(new QGridLayout(this));
    createDiskMaps();
    qApp->restoreOverrideCursor();
}

SummaryWidget::~SummaryWidget()
{
    qDeleteAll(m_disksFolders);
}

void SummaryWidget::createDiskMaps()
{
    DiskList disks;

    QString text;

    for (DiskList::ConstIterator it = disks.constBegin(), end = disks.constEnd(); it != end; ++it)
    {
        Disk const &disk = it.value();

        if (disk.free == 0 && disk.used == 0) {
            continue;
        }

        QWidget *volume = new QWidget(this);
        QVBoxLayout *volumeLayout = new QVBoxLayout(volume);
        RadialMap::Widget *map = new MyRadialMap(this);
        connect(this, &SummaryWidget::canvasDirtied, map, &RadialMap::Widget::refresh);

        QWidget *info = new QWidget(this);
        info->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        QHBoxLayout* horizontalLayout = new QHBoxLayout(info);

        // Create the text under the radialMap.
        if (disk.name.isEmpty()) {
            text = i18nc("Percent used disk space on the partition", "<b>%1</b><br/>%2% Used", disk.mount, disk.used*100/disk.size);
        } else {
            text = i18nc("Percent used disk space on the partition", "<b>%1: %2</b><br/>%3% Used", disk.name, disk.mount, disk.used*100/disk.size);
        }

        QLabel *label = new QLabel(text, this);
        label->setAlignment(Qt::AlignHCenter);
        horizontalLayout->addWidget(label);

        horizontalLayout->setAlignment(Qt::AlignCenter);
        volumeLayout->addWidget(map);
        volumeLayout->addWidget(info);

        //                                                      row (=n/2)           column (0 or 1)
        qobject_cast<QGridLayout*>(layout())->addWidget(volume, layout()->count()/2, layout()->count() % 2);

        Folder *tree = new Folder(disk.mount.toUtf8().constData());
        tree->append("free", disk.free);
        tree->append("used", disk.used);

        map->create(tree); //must be done when visible
        m_disksFolders.append(tree);

        connect(map, &RadialMap::Widget::activated, this, &SummaryWidget::activated);
    }
}

DiskList::DiskList()
{
    QSet<QByteArray> ignoredFsTypes = { "tmpfs", "squashfs", "autofs" };

    if (!Config::scanRemoteMounts) {
        ignoredFsTypes += Config::remoteFsTypes;
    }

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (!storage.isReady() || ignoredFsTypes.contains(storage.fileSystemType())) {
            continue;
        }

        Disk disk;
        disk.mount = storage.rootPath();
        disk.name = storage.name();
        disk.size = storage.bytesTotal();
        disk.free = storage.bytesAvailable();
        disk.used = disk.size - disk.free;

        // if something is mounted over same path, last mounted point would be used since only it is currently reachable.
        (*this)[disk.mount] = disk;
    }
}

}

#include "summaryWidget.moc"
