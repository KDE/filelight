/***************************************************************************
                          scanmanager.h  -  description
                             -------------------
    begin                : Tue Oct 21 2003
    copyright            : (C) 2003 by Max Howell
    email                : max.howell@methylblue.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SCANMANAGER_H
#define SCANMANAGER_H

#include <qobject.h>
#include <qthread.h>
#include <qevent.h>
#include <qstring.h>

#include "filetree.h"


class KURL;
class ScanThread;

class ScanManager : public QObject
{
Q_OBJECT
  
public: 
    ScanManager( QObject *parent, const char *name );
    ~ScanManager();

    enum ScanError { NoError, InvalidProtocol, InvalidUrl, NotDirectory, UnableToStat };
    
public slots:
    void start( const KURL &, bool = false );
    void abort();
    void emptyCache() { cache.empty(); emit cacheInvalidated(); }

signals:
    void started( const QString & );
    void aborted();
    void cached( const Directory * );
    void succeeded( const Directory * );
    void failed( const QString & ); //**** KURL?
    void cacheInvalidated();

private:
    void startPrivate( const QString &, bool );
    void customEvent( QCustomEvent * e );
    
    ScanThread *m_thread; //stack allocated seems to crash on destruction with Qt < 3.2
    Chain<Directory> cache;

public:
    static bool readMounts();
private:
    static QStringList localMounts, remoteMounts;
};


class ScanThread : public QThread
{
public:
    ScanThread() : m_trees( 0 ), m_parent( 0 ) {}

    void init( const QString &s, QObject *o, Chain<Directory> *l ) { m_path = s; m_parent = o; m_trees  = l; }
    Directory *scan( const QString &, const QString & );

    void run();
    
    friend void ScanManager::startPrivate( const QString &, bool );

private:
    QString m_path;
    Chain<Directory> *m_trees;
    QObject *m_parent;

public:
    static void abort() { bAbort = true; } //**** interface sucks
    static unsigned int filesScanned() { return fileCounter; }
private:
    static bool bAbort;
    static unsigned int fileCounter; //**** name sucks
};


class ScanCompleteEvent : public QCustomEvent
{
  public:
    ScanCompleteEvent( Directory *p ) : QCustomEvent( 65433 ), m_tree( p ) { }
    Directory* tree() const { return m_tree; }
  private:
    Directory* const m_tree;
};


class ScanFailedEvent : public QCustomEvent
{
  public:
    ScanFailedEvent( const QString s ) : QCustomEvent( 65434 ), m_path( s ) { }
    const QString &path() const { return m_path; }
  private:
    const QString m_path;
};

#endif
