// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "fileCleaner.h"

#include <QScopeGuard>
#include <QThread>

void FileCleaner::clean(const QList<std::shared_ptr<File>> &files)
{
    // move into our thread
    QMetaObject::invokeMethod(
        this,
        [this, files] {
            cleanUp(files);
        },
        Qt::QueuedConnection);
}

FileCleaner *FileCleaner::instance()
{
    static auto cleaner = new FileCleaner;
    static QThread thread;
    auto *threadPtr = &thread;
    static auto joinThread = qScopeGuard([threadPtr] { // join the thread before it gets stack unwound
        threadPtr->quit();
        threadPtr->wait();
    });

    static bool started = false;
    if (!started) {
        cleaner->moveToThread(&thread);
        thread.start(QThread::IdlePriority);
    }
    return cleaner;
}

void FileCleaner::cleanUp(QList<std::shared_ptr<File>> files)
{
    files.clear();
}
