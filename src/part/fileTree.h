//Author:    Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright: See COPYING file that comes with this distribution

#ifndef FILETREE_H
#define FILETREE_H

#include <qcstring.h> //qstrdup
#include <qfile.h>    //decodeName()
#include <stdlib.h>


//TODO these are pointlessly general purpose now, make them incredibly specific



typedef unsigned long int FileSize;
typedef unsigned long int Dirsize;  //**** currently unused

template <class T> class Iterator;
template <class T> class ConstIterator;
template <class T> class Chain;

template <class T>
class Link
{
public:
   Link( T* const t ) : prev( this ), next( this ), data( t ) {}
   Link() : prev( this ), next( this ), data( 0 ) {}

//TODO unlinking is slow and you don't use it very much in this context.
//  ** Perhaps you can make a faster deletion system that doesn't bother tidying up first
//  ** and then you MUST call some kind of detach() function when you remove elements otherwise
   ~Link() { delete data; unlink(); }

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
   virtual ~Chain() { empty(); }

   void append( T* const data )
   {
      Link<T>* const link = new Link<T>( data );

      link->prev = head.prev;
      link->next = &head;

      head.prev->next = link;
      head.prev = link;
   }

   void transferTo( Chain &c )
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

   Iterator<T>      iterator()      const { return Iterator<T>( head.next ); }
   ConstIterator<T> constIterator() const { return ConstIterator<T>( head.next ); }
   const Link<T>   *end()           const { return &head; }
   bool             isEmpty()       const { return head.next == &head; }

private:
   Link<T> head;
   void operator=( const Chain& );
};


class Directory;
class QString;

class File
{
public:
   friend class Directory;

   enum UnitPrefix { kilo, mega, giga, tera };

   static const uint DENOMINATOR[4];

public:
   File( const char *name, FileSize size ) : m_parent( 0 ), m_name( qstrdup( name ) ), m_size( size ) {}
   virtual ~File() { delete [] m_name; }

   const Directory *parent() const { return m_parent; }
   const char *name8Bit() const { return m_name; }
   const FileSize size() const { return m_size; }
   QString name() const { return QFile::decodeName( m_name ); }

   virtual bool isDirectory() const { return false; }

   QString fullPath( const Directory* = 0 ) const;
   QString humanReadableSize( UnitPrefix key = mega ) const;

public:
   static QString humanReadableSize( uint size, UnitPrefix Key = mega );

protected:
   File( const char *name, FileSize size, Directory *parent ) : m_parent( parent ), m_name( qstrdup( name ) ), m_size( size ) {}

   Directory *m_parent; //0 if this is treeRoot
   char      *m_name;
   FileSize   m_size;   //in units of KiB

private:
   File( const File& );
   void operator=( const File& );
};


class Directory : public Chain<File>, public File
{
public:
   Directory( const char *name ) : File( name, 0 ), m_children( 0 ) {} //DON'T pass the full path!

   uint children() const { return m_children; }
   virtual bool isDirectory() const { return true; }

   ///appends a Directory
   void append( Directory *d, const char *name=0 )
   {
      if( name ) {
         delete [] d->m_name;
         d->m_name = qstrdup( name ); } //directories that had a fullpath copy just their names this way

      m_children += d->children(); //doesn't include the dir itself
      d->m_parent = this;
      append( (File*)d ); //will add 1 to filecount for the dir itself
   }

   ///appends a File
   void append( const char *name, FileSize size )
   {
      append( new File( name, size, this ) );
   }

private:
   void append( File *p )
   {
      m_children++;
      m_size += p->size();
      Chain<File>::append( p );
   }

   uint m_children;

private:
   Directory( const Directory& ); //undefined
   void operator=( const Directory& ); //undefined
};

#endif
