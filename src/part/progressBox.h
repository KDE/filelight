// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#ifndef PROGRESS_BOX_H
#define PROGRESS_BOX_H

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
