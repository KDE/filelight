//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#ifndef PROGRESSBOX_H
#define PROGRESSBOX_H

#include <qlabel.h>
#include <qtimer.h>

namespace KIO { class Job; }


class ProgressBox : public QLabel
{
Q_OBJECT

public:
    ProgressBox( QWidget*, QObject* );

    void setText( int );

public slots:
    void start();
    void report();
    void stop();
    void halt();

private:
    QTimer m_timer;
};

#endif
