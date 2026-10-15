// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "contextMenuContext.h"

#include <QClipboard>
#include <QDir>
#include <QGuiApplication>
#include <QMimeData>

#include <KCoreDirLister>
#include <KGuiItem>
#include <KIO/DeleteJob>
#include <KIO/ListJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KTerminalLauncherJob>
#include <KUrlMimeData>

#include "Config.h"
#include "filelight_debug.h"
#include "radialMap/map.h"
#include "radialMap/radialMap.h"

#if HAVE_LIBBTRFSUTIL
#include <btrfsutil.h>
#endif

using namespace Qt::StringLiterals;

namespace Filelight
{

void ContextMenuContext::openTerminal(RadialMap::Segment *segment)
{
    openTerminal(segment->url());
}

void ContextMenuContext::openTerminal(const QUrl &url)
{
    Q_ASSERT(url.scheme() == "file"_L1); // terminals generally only support local paths
    auto *job = new KTerminalLauncherJob(QString(), this);
    job->setWorkingDirectory(url.toLocalFile());
    job->start();
}

void ContextMenuContext::doNotScan(RadialMap::Segment *segment)
{
    doNotScan(segment->url());
}

void ContextMenuContext::doNotScan(const QUrl &url)
{
    if (!Config::instance()->skipList.contains(url.toLocalFile())) {
        Config::instance()->skipList.append(url.toLocalFile());
        Config::instance()->write();
    }
}

void ContextMenuContext::copyClipboard(RadialMap::Segment *segment)
{
    copyClipboard(segment->url());
}

void ContextMenuContext::copyClipboard(const QUrl &url)
{
    auto mime = new QMimeData;
    mime->setUrls({url});
    KUrlMimeData::exportUrlsToPortal(mime);
    QGuiApplication::clipboard()->setMimeData(mime, QClipboard::Clipboard);
}

static bool shouldDelete(const QUrl &url, bool isFolder, bool hasSnapshots, bool *shouldDeleteSnapshots)
{
    const QString message = isFolder ? i18n("<qt>The folder at <i>'%1'</i> will be <b>recursively</b> and <b>permanently</b> deleted.</qt>", url.toString())
                                     : i18n("<qt><i>'%1'</i> will be <b>permanently</b> deleted.</qt>", url.toString());

    if (hasSnapshots && HAVE_LIBBTRFSUTIL) {
        const QString snapshotMessage = i18nc("@option:check", "Also delete copies in snapshots");

        auto dialog = new QDialog();

        auto dialogButtons = new QDialogButtonBox(dialog);
        dialogButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        KGuiItem::assign(dialogButtons->button(QDialogButtonBox::Ok), KStandardGuiItem::del());
        KGuiItem::assign(dialogButtons->button(QDialogButtonBox::Cancel), KStandardGuiItem::cancel());

        const auto userIntention = KMessageBox::createKMessageBox(dialog,
                                                                  dialogButtons,
                                                                  QMessageBox::Icon::Warning,
                                                                  message,
                                                                  {},
                                                                  snapshotMessage,
                                                                  shouldDeleteSnapshots,
                                                                  KMessageBox::Dangerous);

        qDebug() << userIntention;
        return userIntention == QDialogButtonBox::StandardButton::Ok;
    }

    const auto userIntention = KMessageBox::warningContinueCancel(nullptr, message, QString(), KGuiItem(i18n("&Delete"), QStringLiteral("edit-delete")));

    return userIntention == KMessageBox::Continue;
}

void ContextMenuContext::deleteFileFromSegment(RadialMap::Segment *segment)
{
    deleteFile(segment->file());
}

void ContextMenuContext::deleteFile(const std::shared_ptr<File> &file)
{
    const auto url = file->url();
    const auto isFolder = file->isFolder();

    QUrl snapshotsUrl;
    snapshotsUrl.setScheme("snapshot"_L1);
    snapshotsUrl.setPath("/file/"_L1 + file->parent()->url().path());
    auto *snapshotLister = new KCoreDirLister(this);
    snapshotLister->openUrl(snapshotsUrl);
    connect(snapshotLister, &KCoreDirLister::completed, this, [this, url, snapshotLister, file, isFolder] {
        QList<QString> snapshots;
        for (const auto &item : snapshotLister->items()) {
            if (QDir::cleanPath(item.mostLocalUrl().path()) != QDir::cleanPath(file->parent()->url().path())) {
                snapshots << item.mostLocalUrl().path();
            }
        }

        bool shouldDeleteSnapshots = true;
        if (!shouldDelete(url, isFolder, !snapshots.empty(), &shouldDeleteSnapshots)) {
            snapshotLister->deleteLater();
            return;
        }

        auto job = KIO::del(url);
        connect(job, &KJob::finished, this, [this, url, job, file] {
            QGuiApplication::restoreOverrideCursor();
            setDeleting(false);
            if (!job->error()) {
                file->parent()->remove(file);
                RadialMap::Map::instance()->refresh(Dirty::Layout);
            } else {
                Q_EMIT deleteFileFailed(i18nc("@info:notification", "Error while deleting: %1", job->errorString()));
            }
        });

#if HAVE_LIBBTRFSUTIL
        if (shouldDeleteSnapshots) {
            for (const auto &snapshot : std::as_const(snapshots)) {
                if (!QDir(snapshot).exists(file->decodedName())) {
                    continue;
                }
                QUrl fileInSnapshot;
                fileInSnapshot.setScheme("file"_L1);
                fileInSnapshot.setPath(QDir(snapshot).absoluteFilePath(file->decodedName()));

                QDir subvolumeRoot(snapshot);

                struct btrfs_util_subvolume_info subvolume_root_info;
                enum btrfs_util_error btrfs_err;

                while ((btrfs_err = btrfs_util_subvolume_get_info(subvolumeRoot.absolutePath().toLocal8Bit().constData(), 0, &subvolume_root_info)) != 0) {
                    if (subvolumeRoot.isRoot()) {
                        break;
                    }
                    bool ok = subvolumeRoot.cdUp();
                    if (!ok) {
                        break;
                    }
                }

                if (btrfs_err != 0) {
                    continue;
                }

                bool oldReadOnly;
                btrfs_err = btrfs_util_subvolume_get_read_only(subvolumeRoot.absolutePath().toLocal8Bit().constData(), &oldReadOnly);
                if (btrfs_err != 0) {
                    qCWarning(FILELIGHT_LOG) << "could not get snapshot read-only status:" << btrfs_util_strerror(btrfs_err);
                }

                btrfs_err = btrfs_util_subvolume_set_read_only(subvolumeRoot.absolutePath().toLocal8Bit().constData(), false);
                if (btrfs_err != 0) {
                    qCWarning(FILELIGHT_LOG) << "could not set snapshot read-only to false:" << btrfs_util_strerror(btrfs_err);
                }

                auto job = KIO::del(fileInSnapshot, KIO::HideProgressInfo);
                connect(job, &KJob::finished, this, [job, subvolumeRoot, oldReadOnly] {
                    if (job->error()) {
                        qCWarning(FILELIGHT_LOG) << "could not delete from snapshot" << subvolumeRoot << job->errorString() << job->errorText();
                    }
                    enum btrfs_util_error btrfs_err = btrfs_util_subvolume_set_read_only(subvolumeRoot.absolutePath().toLocal8Bit().constData(), oldReadOnly);
                    if (btrfs_err != 0) {
                        qCWarning(FILELIGHT_LOG) << "could not restore snapshot read-only status:" << btrfs_util_strerror(btrfs_err);
                    }
                });
            }
        }
#endif

        setDeleting(true);
        QGuiApplication::setOverrideCursor(Qt::BusyCursor);
        snapshotLister->deleteLater();
    });
}

void ContextMenuContext::setDeleting(bool status)
{
    m_deleting = status;
    Q_EMIT deletingChanged();
}

} // namespace Filelight
