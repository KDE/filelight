/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
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

#include "summaryWidget.h"

#include "Config.h"
#include "fileTree.h"
#include "radialMap/radialMap.h"
#include "radialMap/widget.h"



#include <KLocalizedString>

#include <QLabel>
#include <QApplication>
#include <QIcon>
#include <QByteArray>
#include <QList>
#include <QMouseEvent>
#include <QLayout>
#include <QStorageInfo>

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


struct DiskList : QList<Disk>
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
        if (focusSegment() && focusSegment()->file()->name() == QLatin1String( "Used" ))
            RadialMap::Widget::setCursor(c);
        else
            unsetCursor();
    }

    virtual void mousePressEvent(QMouseEvent *e)
    {
        const RadialMap::Segment *segment = focusSegment();

        //we will allow right clicks to the center circle
        if (segment == rootSegment() && e->button() == Qt::RightButton)
            RadialMap::Widget::mousePressEvent(e);

        //and clicks to the used segment
        else if (e->button() == Qt::LeftButton ) {
            const QRect rect(e->x() - 20, e->y() - 20, 40, 40);
//            KIconEffect::visualActivate(this, rect); TODO: Re-enable
            emit activated(url());
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

void SummaryWidget::createDiskMaps()
{
    DiskList disks;

    const QByteArray free = i18nc("Free space on the disks/partitions", "Free").toUtf8();
    const QByteArray used = i18nc("Used space on the disks/partitions", "Used").toUtf8();

    QString text;

    for (DiskList::ConstIterator it = disks.constBegin(), end = disks.constEnd(); it != end; ++it)
    {
        Disk const &disk = *it;

        if (disk.free == 0 && disk.used == 0)
            continue;

        QWidget *volume = new QWidget(this);
        QVBoxLayout *volumeLayout = new QVBoxLayout(volume);
        RadialMap::Widget *map = new MyRadialMap(this);

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

        Folder *tree = new Folder(disk.mount.toUtf8());
        tree->append(free, disk.free);
        tree->append(used, disk.used);

        map->create(tree); //must be done when visible

        connect(map, &RadialMap::Widget::activated, this, &SummaryWidget::activated);
    }
}

DiskList::DiskList()
{
    QStringList partitions;

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (!storage.isReady() || storage.fileSystemType() == "tmpfs") {
            continue;
        }

        Disk disk;
        disk.mount = storage.rootPath();
        disk.name = storage.name();
        disk.size = storage.bytesTotal();
        disk.free = storage.bytesFree();
        disk.used = disk.size - disk.free;

        *this += disk;
    }
}

}

#include "summaryWidget.moc"