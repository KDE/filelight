// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "posixWalker.h"

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

        m_entry.isSkipable =
            S_ISLNK(statbuf.st_mode) || S_ISCHR(statbuf.st_mode) || S_ISBLK(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode) || S_ISSOCK(statbuf.st_mode);
        m_entry.isDir = S_ISDIR(statbuf.st_mode);
        m_entry.isFile = S_ISREG(statbuf.st_mode);
        if (Q_UNLIKELY(statbuf.st_blocks == 0 && statbuf.st_size != 0)) { // some fuse implementations don't return blocks; fall back to size
            m_entry.size = statbuf.st_size;
        } else { // otherwise default to the actual size in blocks
            m_entry.size = statbuf.st_blocks * S_BLKSIZE;
        }
        break;
    }
}
