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

#ifndef FILETREE_H
#define FILETREE_H

#include <QByteArray> //qstrdup
#include <QFile> //decodeName()
#include <QDebug>
#include <QLocale>
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
    File(const char *name, FileSize size) : m_parent(0), m_name(qstrdup(name)), m_size(size) {}
    virtual ~File() {
        delete [] m_name;
    }

    Folder *parent() const {
        return m_parent;
    }
    const char *name8Bit() const {
        return m_name;
    }
    FileSize size() const {
        return m_size;
    }
    QString name() const {
        return QFile::decodeName(m_name);
    }

    virtual bool isFolder() const {
        return false;
    }

    QString fullPath(const Folder* = 0) const;
    QString humanReadableSize() const {
        return KFormat().formatByteSize(m_size);
    }

protected:
    File(const char *name, FileSize size, Folder *parent) : m_parent(parent), m_name(qstrdup(name)), m_size(size) {}

    Folder *m_parent; //0 if this is treeRoot
    char      *m_name;
    FileSize   m_size;   //in units of KiB

private:
    File(const File&);
    void operator=(const File&);
};


class Folder : public File
{
public:
    Folder(const char *name) : File(name, 0), m_children(0) {} //DON'T pass the full path!

    uint children() const {
        return m_children;
    }
    virtual bool isFolder() const {
        return true;
    }

    ///appends a Folder
    void append(Folder *d, const char *name=0)
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
        
        for (Folder *d = this; d; d = d->parent()) {
            d->m_size -= f->size();
        }
    }

    QList<File *> files;

private:
    void append(File *p)
    {
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
