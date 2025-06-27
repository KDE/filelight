// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "uringWalker.h"
#include <QDebug>

#ifdef Q_OS_LINUX
#include <sys/param.h>
#include <liburing.h>
#endif

static void outputError(const QByteArray &path)
{
    /// show error message that stat or opendir may give

#define out(s)                                                                                                                                                 \
    qWarning() << s ": " << path;                                                                                                                              \
    break

    switch (errno) {
    case EACCES:
        out("Inadequate access permissions");
    case EMFILE:
        out("Too many file descriptors in use by Filelight");
    case ENFILE:
        out("Too many files are currently open in the system");
    case ENOENT:
        out("A component of the path does not exist, or the path is an empty string");
    case ENOMEM:
        out("Insufficient memory to complete the operation");
    case ENOTDIR:
        out("A component of the path is not a folder");
    case EBADF:
        out("Bad file descriptor");
    case EFAULT:
        out("Bad address");
    case ELOOP: // NOTE shouldn't ever happen
        out("Too many symbolic links encountered while traversing the path");
    case ENAMETOOLONG:
        out("File name too long");
    }

#undef out
}

URingWalker::URingWalker(const QByteArray &path)
    : m_path(path)
{
    if (path.isEmpty()) {
        return;
    }

    io_uring_queue_init(128, &m_ring, 0);

    m_dir = opendir(path.constData());
    if (!m_dir) {
        outputError(QByteArray(path));
        return;
    }

    m_dirfd = dirfd(m_dir);
    if (m_dirfd < 0) {
        qWarning() << "dirfd failed";
        outputError(QByteArray(path));
        return;
    }
    // load first entry to achieve iterator behavior. If there are no entries then this results
    // in a default constructed m_entry and thus ==end(); otherwise it is the first m_entry ==begin().
    next();
}

URingWalker::~URingWalker()
{
    close();
    io_uring_queue_exit(&m_ring);
}

void URingWalker::close()
{
    if (m_dir) {
        closedir(m_dir);
        m_dir = nullptr;
        m_dirfd = -1;
    }
}

void URingWalker::next()
{
    m_entry = {}; // reset

    if (!m_dir || m_dirfd < 0) {
        return;
    }

    while (true) {
        auto ent = readdir(m_dir);
        if (!ent) { // end of dir
            break;
        }

        if (qstrcmp(ent->d_name, ".") == 0 || qstrcmp(ent->d_name, "..") == 0) {
            continue;
        }

        struct io_uring_sqe *sqe = io_uring_get_sqe(&m_ring);
        if (!sqe) { // No more spots, proceed with completion
            break;
        }

        auto node = new Node{.sqe = sqe, .entry = {}}; // awkwardly dangling until we fish it out of the CQE below
        node->entry.name = QByteArray(ent->d_name);
        io_uring_prep_statx(sqe, m_dirfd, ent->d_name, AT_SYMLINK_NOFOLLOW, STATX_MODE | STATX_NLINK | STATX_INO | STATX_BLOCKS, &node->statbuf);
        io_uring_sqe_set_data(sqe, node);
        m_pending++;
    }

    io_uring_submit(&m_ring);

    if (m_pending <= 0) {
        qDebug() << "no more pending";
        close();
        return;
    }

    struct io_uring_cqe *cqe = nullptr;
    int ret = io_uring_wait_cqe(&m_ring, &cqe);
    auto seen = qScopeGuard([this, cqe] {
        io_uring_cqe_seen(&m_ring, cqe);
        m_pending--;
    });
    if (ret < 0) {
        qWarning() << "io_uring_wait_cqe failed" << strerror(-ret);
        return;
    }

    auto node = std::unique_ptr<Node>(static_cast<Node *>(io_uring_cqe_get_data(cqe)));
    m_entry = node->entry;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outputError(node->entry.name);
        return;
    }

    auto &statbuf = node->statbuf;

    m_entry.isSkipable =
        S_ISLNK(statbuf.stx_mode) || S_ISCHR(statbuf.stx_mode) || S_ISBLK(statbuf.stx_mode) || S_ISFIFO(statbuf.stx_mode) || S_ISSOCK(statbuf.stx_mode);

    auto links = statbuf.stx_nlink;
    // Only count as hard link if it's not already being skipped
    if (links > 1 && !m_entry.isSkipable) {
        ino_t inode = statbuf.stx_ino;
        // If we already counted this inode, skip it
        if (m_countedHardlinks.find(inode) != m_countedHardlinks.end()) {
            m_entry.isDuplicate = true;
        } else {
            // Only add to counted hard links if we are going to count it
            m_countedHardlinks.insert(inode);
        }
    }
    m_entry.isDir = S_ISDIR(statbuf.stx_mode);
    m_entry.isFile = S_ISREG(statbuf.stx_mode);

#ifdef Q_OS_LINUX
    m_entry.size = statbuf.stx_blocks * DEV_BSIZE;
#else
    m_entry.size = statbuf.stx_blocks * S_BLKSIZE;
#endif
}
