// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#ifndef SUMMARY_WIDGET_H
#define SUMMARY_WIDGET_H

#include <qwidget.h>


class SummaryWidget : public QWidget
{
    Q_OBJECT

public:
    SummaryWidget( QWidget *parent, const char *name );
    ~SummaryWidget();

signals:
    void activated( const KURL& );

private:
    void createDiskMaps();
};

#endif
