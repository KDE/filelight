/***************************************************************************
                          scanbox.h  -  description
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

#ifndef SCANBOX_H
#define SCANBOX_H

#include <qlabel.h>



class QTimer;

class ScanProgressBox : public QLabel
{
  Q_OBJECT

  public:
    ScanProgressBox( QWidget *, const char * );
    virtual ~ScanProgressBox();

  public slots:
    void start();
    void report();
    void stop();    

  private:
    QTimer *m_timer1, *m_timer2;
};
    
#endif
