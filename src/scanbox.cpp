/***************************************************************************
                          scanbox.cpp  -  description
                             -------------------
    begin                : Fri Jul 11 2003
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

#include <qlabel.h>
#include <qtimer.h>

#include <kglobal.h>
#include <klocale.h>

#include "scanmanager.h"
#include "scanbox.h"



ScanProgressBox::ScanProgressBox( QWidget *parent = 0, const char *name = 0 ) : QLabel( parent, name )
{
    QFontMetrics fm = this->fontMetrics();
    setFrameStyle( Box | Raised );
    setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Maximum );
    setMinimumWidth( fm.width( i18n( "%1 Files Scanned" ).arg( KGlobal::locale()->formatNumber( 999999, 0 ) ) ) );

    m_timer1 = new QTimer;
    m_timer2 = new QTimer;
      
    connect( m_timer1, SIGNAL( timeout() ), this, SLOT( report() ) );
    connect( m_timer2, SIGNAL( timeout() ), this, SLOT( hide() ) );    
}


ScanProgressBox::~ScanProgressBox()
{
    delete m_timer1;
    delete m_timer2;
}


void
ScanProgressBox::start()
{
  show();
  m_timer2->stop(); //stop premature hiding
  m_timer1->start( 50 ); //20 times per second - very smooth
  report();
}


void
ScanProgressBox::report()
{
  setText( i18n( "%1 Files Scanned" ).arg( KGlobal::locale()->formatNumber( ScanThread::filesScanned(), 0 ) ));
}


void
ScanProgressBox::stop()
{
  if( !m_timer1->isActive() ) //then scan didn't run, but new map shown still so hide()
    hide();
  else {
    m_timer1->stop();
    report(); //sometimes it's so quick this isn't called even once!
    m_timer2->start( 3000, true );
  }
}

#include "scanbox.moc"
