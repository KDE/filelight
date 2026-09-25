// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "posixWalker.h"
#include <QDebug>
#include <QScopeGuard>

#ifdef Q_OS_LINUX
#include <linux/btrfs.h>
#include <linux/fiemap.h>
#include <linux/fs.h>
#include <linux/magic.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#define MAX_EXTENTS 64
#endif

#ifdef Q_OS_HAIKU
#define S_BLKSIZE 512
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

POSIXWalker::POSIXWalker(const QByteArray &path)
    : m_path(path)
{
    if (path.isEmpty()) {
        return;
    }

    m_dir = opendir(path.constData());
    if (!m_dir) {
        outputError(QByteArray(path));
        return;
    }

    m_dirfd = dirfd(m_dir);

#ifdef Q_OS_LINUX
    struct statfs sfs;
    if (statfs(path.constData(), &sfs) < 0) {
        outputError(QByteArray(path));
        return;
    }
    isBtrfs = (sfs.f_type == BTRFS_SUPER_MAGIC);
#endif

    // load first entry to achieve iterator behavior. If there are no entries then this results
    // in a default constructed m_entry and thus ==end(); otherwise it is the first m_entry ==begin().
    next();
}

POSIXWalker::~POSIXWalker()
{
    close();
}

void POSIXWalker::close()
{
    if (m_dir) {
        closedir(m_dir);
        m_dir = nullptr;
        m_dirfd = -1;
    }
}

void POSIXWalker::next()
{
    while (true) {
        m_entry = {}; // reset

        if (!m_dir) {
            return;
        }

        dirent *ent = readdir(m_dir);
        if (!ent) { // end of dir
            close();
            return;
        }

        if (qstrcmp(ent->d_name, ".") == 0 || qstrcmp(ent->d_name, "..") == 0) {
            continue;
        }

        m_entry.name = QByteArray(ent->d_name);
        if (fstatat(m_dirfd, ent->d_name, &statbuf, AT_SYMLINK_NOFOLLOW) == -1) {
            outputError(m_entry.name);
            return;
        }

        m_entry.isSkippable =
            S_ISLNK(statbuf.st_mode) || S_ISCHR(statbuf.st_mode) || S_ISBLK(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode) || S_ISSOCK(statbuf.st_mode);

        auto links = statbuf.st_nlink;
        // Only count as hard link if it's not already being skipped
        if (links > 1 && !m_entry.isSkippable) {
            ino_t inode = statbuf.st_ino;
            // If we already counted this inode, skip it
            if (m_countedHardlinks.contains(inode)) {
                m_entry.isDuplicate = true;
            } else {
                // Only add to counted hard links if we are going to count it
                m_countedHardlinks.insert(inode);
            }
        }
        m_entry.isDir = S_ISDIR(statbuf.st_mode);
        m_entry.isFile = S_ISREG(statbuf.st_mode);

#ifdef Q_OS_LINUX
        m_entry.size = m_entry.sizeIncludingShared = statbuf.st_blocks * DEV_BSIZE;

        if (isBtrfs && m_entry.isFile) {
            int fd = openat(m_dirfd, ent->d_name, AT_SYMLINK_NOFOLLOW);
            if (fd < 0) {
                outputError(m_entry.name);
            }
            auto fdGuard = qScopeGuard([fd] {
                ::close(fd);
            });

            char buf[sizeof(struct fiemap) + sizeof(struct fiemap_extent) * MAX_EXTENTS];
            struct fiemap *fiemap = (struct fiemap *)buf;
            struct fiemap_extent *extents = &fiemap->fm_extents[0];

            memset(fiemap, 0, sizeof(struct fiemap));

            bool last = false;
            uint64_t total_size = 0;
            uint64_t shared_size = 0;

            fiemap->fm_flags = FIEMAP_FLAG_SYNC;

            do {
                fiemap->fm_length = FIEMAP_MAX_OFFSET;
                fiemap->fm_extent_count = MAX_EXTENTS;

                int rc = ioctl(fd, FS_IOC_FIEMAP, (unsigned long)fiemap);
                if (rc < 0) {
                    outputError(m_entry.name);
                    break;
                }

                if (fiemap->fm_mapped_extents == 0) {
                    break;
                }

                for (uint32_t i = 0; i < fiemap->fm_mapped_extents; i++) {
                    struct fiemap_extent extent = extents[i];
                    if (extent.fe_flags & FIEMAP_EXTENT_LAST) {
                        last = true;
                    }

                    if (extent.fe_flags & (FIEMAP_EXTENT_DATA_INLINE | FIEMAP_EXTENT_UNKNOWN | FIEMAP_EXTENT_DELALLOC)) {
                        continue;
                    }

                    if (extent.fe_length == 0) {
                        continue;
                    }

                    total_size += extent.fe_length;
                    if (extent.fe_flags & FIEMAP_EXTENT_SHARED) {
                        shared_size += extent.fe_length;
                    }
                }

                uint32_t last_ext = fiemap->fm_mapped_extents - 1;
                fiemap->fm_start = extents[last_ext].fe_logical + extents[last_ext].fe_length;
            } while (!last);

            m_entry.size = total_size - shared_size;
            m_entry.sizeIncludingShared = shared_size;
        }
#else
        m_entry.size = m_entry.sizeIncludingShared = statbuf.st_blocks * S_BLKSIZE;
#endif
        break;
    }
}
