// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "contextMenuContext.h"

#include <QClipboard>
#include <QCursor>
#include <QGuiApplication>
#include <QMimeData>

#include <KIO/DeleteJob>
#include <KLocalizedString>
#include <KTerminalLauncherJob>
#include <KUrlMimeData>

#include "Config.h"
#include "radialMap/map.h"
#include "radialMap/radialMap.h"

namespace Filelight
{

void ContextMenuContext::openTerminal(RadialMap::Segment *segment)
{
    openTerminal(segment->url());
}

void ContextMenuContext::openTerminal(const QUrl &url)
{
    auto *job = new KTerminalLauncherJob(QString(), this);
    job->setWorkingDirectory(url.path());
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

void ContextMenuContext::deleteFile(const FileWrapper &_file)
{
    const auto file = _file.file();
    const auto url = file->url();
    const auto isFolder = file->isFolder();

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
    setDeleting(true);
    QGuiApplication::setOverrideCursor(Qt::BusyCursor);
}

void ContextMenuContext::setDeleting(bool status)
{
    m_deleting = status;
    Q_EMIT deletingChanged();
}

} // namespace Filelight
