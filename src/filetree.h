/***************************************************************************
                          filetree.h  -  description
                             -------------------
    begin                : Thu Jun 12 2003
    copyright            : (C) 2003 by Max Howell
    email                : filelight@methylblue.com

    contents             : classes for creating a memory map of filetrees
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FILETREE_H
#define FILETREE_H

typedef unsigned long int Filesize;
typedef unsigned long int Dirsize;  //**** currently unused



template <class T> class Iterator;
template <class T> class ConstIterator;
template <class T> class Chain;

template <class T>
class Link
{
public:
    Link( T* const t ) : prev( this ), next( this ), data( t ) { }
    Link() : prev( this ), next( this ), data( 0 ) { }

  //**** unlinking is slow and you don't use it very much in this context.
  //  ** Perhaps you can make a faster deletion system that doesn't bother tidying up first
  //  ** and then you MUST call some kind of detach() function when you remove elements otherwise
    virtual ~Link() { delete data; unlink(); }

    friend class Iterator<T>;
    friend class ConstIterator<T>;    
    friend class Chain<T>;

private:
    void unlink() { prev->next = next; next->prev = prev; prev = next = this; }

    Link<T>* prev;
    Link<T>* next;

    T* data; //ensure only iterators have access to this
};


template <class T>
class Iterator
{
public:
    Iterator() : link( 0 ) { } //**** remove this, remove this REMOVE THIS!!! dangerous as your implementation doesn't test for null links, always assumes they can be derefenced
    Iterator( Link<T> *p ) : link( p ) { }

    bool operator==( const Iterator<T>& it ) const { return link == it.link; }
    bool operator!=( const Iterator<T>& it ) const { return link != it.link; }
    bool operator!=( const Link<T> *p ) const { return p != link; }

    //here we have a choice, really I should make two classes one const the other not
    const T* operator*() const { return link->data; }
    T* operator*() { return link->data; }
    
    Iterator<T>& operator++() { link = link->next; return *this; } //**** does it waste time returning in places where we don't use the retval?

    bool isNull() const { return (link == 0); } //REMOVE WITH ABOVE REMOVAL you don't want null iterators to be possible
    
    void transferTo( Chain<T> &chain )
    {
      chain.append( remove() );
    }
    
    T* const remove() //remove from list, delete Link, data is returned NOT deleted
    {
      T* const d = link->data;
      Link<T>* const p = link->prev;

      link->data = 0;
      delete link;
      link = p; //make iterator point to previous element, YOU must check this points to an element

      return d;
    }
    
private:
    Link<T> *link;
};


template <class T>
class ConstIterator
{
public:
    ConstIterator( Link<T> *p ) : link( p ) { }

    bool operator==( const Iterator<T>& it ) const { return link == it.link; }
    bool operator!=( const Iterator<T>& it ) const { return link != it.link; }
    bool operator!=( const Link<T> *p ) const { return p != link; }

    const T* operator*() const { return link->data; }

    ConstIterator<T>& operator++() { link = link->next; return *this; }

private:
    const Link<T> *link;
};


template <class T>
class Chain
{
public:
    Chain() { }
    virtual ~Chain() { empty(); }

    void append( T* const data )
    {
      Link<T>* const link = new Link<T>( data );

      link->prev = head.prev;
      link->next = &head;

      head.prev->next = link;
      head.prev = link;
    }

    void transferTo ( Chain &c )
    {
      if( isEmpty() ) return;

      Link<T>* const first = head.next;
      Link<T>* const last  = head.prev;      

      head.unlink();
      
      first->prev = c.head.prev;
      c.head.prev->next = first;

      last->next = &c.head;
      c.head.prev = last;
    }

    void empty() { while( head.next != &head ) { delete head.next; } }

    Iterator<T>      iterator() const { return Iterator<T>( head.next ); }
    ConstIterator<T> constIterator() const { return ConstIterator<T>( head.next ); }
    const Link<T>   *end() const { return &head; }
    bool             isEmpty() const { return ( head.next == &head ); }

private:
    Link<T> head;
};


class Directory;

class File
{
public:
    File( const char *n, const Filesize s ) : m_parent( 0 ), m_name( n ), m_size( s ) { }
//    File( const char *n, const Directory *p, const Filesize s ) : m_parent( p ), m_name( n ), m_size( s ) { }    
    virtual ~File() { delete [] m_name; }

    const   Directory* parent() const { return m_parent; }
    const   Filesize     size() const { return m_size; }
    const   char*        name() const { return m_name; }
    virtual bool        isDir() const { return false; }

    friend class Directory; //**** too broad, but never mind I spose
    
protected:
    const Directory* m_parent; //NULL if this is treeRoot
    const char* m_name;
    Filesize m_size;   //use units of kB
};


class Directory : public Chain<File>, public File
{
public:
    Directory( const char *filename ) : File( filename, 0 ), m_fileCount( 0 ) { } //DON'T pass the full path!
    virtual ~Directory() { }

    //overides Chain
    void append( Directory *p )
    {
      m_fileCount += p->fileCount(); //we also need to add one for the dir.
      append( (File*)p ); //will add 1 to filecount for the dir itself
    }

    void append( File *p )
    {
      ++m_fileCount;
      m_size += p->size();
      p->m_parent = this;
      Chain<File>::append( p );
    }

  //**** this SUCKS! but what to do? See ScanThread::scan()
    void setName( char *p ) { delete [] m_name; m_name = p; }
    
    unsigned int fileCount() const { return m_fileCount; }
    virtual bool isDir()     const { return true; }

private:
    unsigned int m_fileCount;
};

#endif
