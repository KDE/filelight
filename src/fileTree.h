/***********************************************************************
 * SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
 * SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
 * SPDX-FileCopyrightText: 2017-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***********************************************************************/

#pragma once

#include <cstdlib>

#include <KFormat>

#include <QByteArray>
#include <QDebug>

using FileSize = quint64;

class Folder;
namespace RadialMap
{
class Segment;
} // namespace RadialMap

class File
{
    friend class Folder;

public:
    File(const char *name, FileSize size)
        : m_parent(nullptr)
        , m_name(name)
        , m_size(size)
    {
    }
    virtual ~File() = default;
    File(const File &) = default;
    File &operator=(const File &) = default;
    File(File &&) = default;
    File &operator=(File &&) = default;

    QString segment() const
    {
        return m_segment;
    }

    void setSegment(const QString &segment)
    {
        Q_ASSERT(m_segment.isEmpty() || segment.isEmpty());
        m_segment = segment;
    }

    Folder *parent() const
    {
        return m_parent;
    }

    /** Do not use for user visible strings. Use name instead. */
    const char *name8Bit() const
    {
        return m_name.constData();
    }
    void setName(const QByteArray &newName)
    {
        m_name = newName;
    }
    /** Decoded name. Use when you need a QString. */
    QString decodedName() const
    {
        return QString::fromUtf8(m_name);
    }
    /**
     * Human readable name (including native separators where applicable).
     * Only use for display.
     */
    QString displayName() const;

    FileSize size() const
    {
        return m_size;
    }

    virtual bool isFolder() const
    {
        return false;
    }

    /**
     * Human readable path for display (including native separators where applicable.
     * Only use for display.
     */
    QString displayPath(const std::shared_ptr<Folder> &root = {}) const;
    QString humanReadableSize() const
    {
        return KFormat().formatByteSize(m_size);
    }

    /** Builds a complete QUrl by walking up to root. */
    QUrl url(const std::shared_ptr<Folder> &root = {}) const;

protected:
    File(const char *name, FileSize size, Folder *parent)
        : m_parent(parent)
        , m_name(name)
        , m_size(size)
    {
    }

    Folder *m_parent; // 0 if this is treeRoot; this is a non-owning pointer, the parent owns "us"
    QByteArray m_name; // partial path name (e.g. 'boot/' or 'foo.svg')
    FileSize m_size; // in units of bytes; sum of all children's sizes

    QString m_segment;
};

class Folder : public File
{
public:
    explicit Folder(const char *name)
        : File(name, 0)
    {
    } // DON'T pass the full path!

    ~Folder();
    Folder(const Folder &) = default;
    Folder &operator=(const Folder &) = default;
    Folder(Folder &&) = default;
    Folder &operator=(Folder &&) = default;

    uint children() const
    {
        return m_children;
    }
    bool isFolder() const override
    {
        return true;
    }

    std::shared_ptr<Folder> duplicate() const
    {
        return std::make_shared<Folder>(*this);
    }

    /// appends a Folder
    void append(const std::shared_ptr<Folder> &d, const char *name = nullptr)
    {
        if (name) {
            d->m_name = name;
        } // directories that had a fullpath copy just their names this way

        m_children += d->children(); // doesn't include the dir itself
        Q_ASSERT(d->m_parent == this || d->m_parent == nullptr);
        d->m_parent = this;

        appendFile(d); // will add 1 to filecount for the dir itself
    }

    /// appends a File
    void append(const char *name, FileSize size)
    {
        appendFile(std::shared_ptr<File>(new File(name, size, this)));
    }

    /// removes a file
    void remove(const std::shared_ptr<File> &f)
    {
        files.removeAll(f);
        const FileSize childSize = f->size();
        for (Folder *d = this; d; d = d->parent()) {
            d->m_size -= childSize;
            d->m_children--;
        }
    }

    // Removes a file, but does not delete
    std::shared_ptr<Folder> take(const std::shared_ptr<Folder> &f)
    {
        files.removeAll(f);
        const FileSize childSize = f->size();
        for (Folder *d = this; d; d = d->parent()) {
            d->m_size -= childSize;
            d->m_children--;
        }
        return f;
    }

    QList<std::shared_ptr<File>> files;

private:
    void appendFile(const std::shared_ptr<File> &p)
    {
        // This is also called by append(Folder), but only once all its children
        // were scanned. We do not need to forward the size change to our parent
        // since in turn we too only are added to our parent when we are have
        // been scanned already.
        m_children++;
        m_size += p->size();
        files.append(p);
    }

    uint m_children = 0;
};
