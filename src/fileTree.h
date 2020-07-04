/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
* SPDX-FileCopyrightText: 2017 Harald Sitter <sitter@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#ifndef FILETREE_H
#define FILETREE_H

#include <QByteArray> //qstrdup
#include <QFile> //decodeName()
#include <KFormat>

#include <stdlib.h>

typedef quint64 FileSize;
typedef quint64 Dirsize;  //**** currently unused

class Folder;

class File
{
public:
    friend class Folder;

public:
    File(const char *name, FileSize size) : m_parent(nullptr), m_name(qstrdup(name)), m_size(size) {}
    virtual ~File() {
        delete [] m_name;
    }

    Folder *parent() const {
        return m_parent;
    }

    /** Do not use for user visible strings. Use name instead. */
    const char *name8Bit() const {
        return m_name;
    }
    void setName(const QByteArray &newName) {
        delete [] m_name;
        m_name = qstrdup(newName.constData());
    }
    /** Decoded name. Use when you need a QString. */
    QString decodedName() const {
        return QFile::decodeName(m_name);
    }
    /**
     * Human readable name (including native separators where applicable).
     * Only use for display.
     */
    QString displayName() const;

    FileSize size() const {
        return m_size;
    }

    virtual bool isFolder() const {
        return false;
    }

    /**
     * Human readable path for display (including native separators where applicable.
     * Only use for display.
     */
    QString displayPath(const Folder * = nullptr) const;
    QString humanReadableSize() const {
        return KFormat().formatByteSize(m_size);
    }

    /** Builds a complete QUrl by walking up to root. */
    QUrl url(const Folder *root = nullptr) const;

protected:
    File(const char *name, FileSize size, Folder *parent) : m_parent(parent), m_name(qstrdup(name)), m_size(size) {}

    Folder *m_parent; //0 if this is treeRoot
    char *m_name; // partial path name (e.g. 'boot/' or 'foo.svg')
    FileSize m_size; // in units of bytes; sum of all children's sizes

private:
    File(const File&);
    void operator=(const File&);
};


class Folder : public File
{
public:
    Folder(const char *name) : File(name, 0), m_children(0) {} //DON'T pass the full path!

    ~Folder()
    {
        qDeleteAll(files);
    }

    uint children() const {
        return m_children;
    }
    bool isFolder() const override {
        return true;
    }

    Folder *duplicate() const
    {
        Folder *ret = new Folder(m_name);
        for (const File *child : files) {
            if (child->isFolder()) {
                ret->append(((Folder*)child)->duplicate());
            } else {
                ret->append(child->name8Bit(), child->size());
            }
        }
        return ret;
    }

    ///appends a Folder
    void append(Folder *d, const char *name=nullptr)
    {
        if (name) {
            delete [] d->m_name;
            d->m_name = qstrdup(name);
        } //directories that had a fullpath copy just their names this way

        m_children += d->children(); //doesn't include the dir itself
        d->m_parent = this;
        append((File*)d); //will add 1 to filecount for the dir itself
    }

    ///appends a File
    void append(const char *name, FileSize size)
    {
        append(new File(name, size, this));
    }

    /// removes a file
    void remove(const File *f) {
        files.removeAll(const_cast<File*>(f));
        const FileSize childSize = f->size();
        delete f;

        for (Folder *d = this; d; d = d->parent()) {
            d->m_size -= childSize;
            d->m_children--;
        }
    }

    // Removes a file, but does not delete
    const File *take(const File *f) {
        files.removeAll(const_cast<File*>(f));
        const FileSize childSize = f->size();

        for (Folder *d = this; d; d = d->parent()) {
            d->m_size -= childSize;
            d->m_children--;
        }
        return f;
    }

    QList<File *> files;

private:
    void append(File *p)
    {
        // This is also called by append(Folder), but only once all its children
        // were scanned. We do not need to forward the size change to our parent
        // since in turn we too only are added to our parent when we are have
        // been scanned already.
        m_children++;
        m_size += p->size();
        files.append(p);
    }

    uint m_children;

private:
    Folder(const Folder&); //undefined
    void operator=(const Folder&); //undefined
};

#endif
