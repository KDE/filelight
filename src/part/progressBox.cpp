// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#include <kglobal.h>
#include <kglobalsettings.h>
#include <kio/job.h>
#include <klocale.h>

#include "scan.h"
#include "progressBox.h"


ProgressBox::ProgressBox( QWidget *parent, QObject *part )
   : QLabel( parent, "ProgressBox" )
{
   hide();

   setAlignment( Qt::AlignCenter );
   setFont( KGlobalSettings::fixedFont() );
   setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );

   setText( 999999 );
   setMinimumWidth( sizeHint().width() );

   connect( &m_timer, SIGNAL(timeout()), SLOT(report()) );
   connect( part, SIGNAL(started( KIO::Job* )), SLOT(start()) );
   connect( part, SIGNAL(completed()), SLOT(stop()) );
   connect( part, SIGNAL(canceled( const QString& )), SLOT(halt()) );
}

void
ProgressBox::start() //slot
{
   m_timer.start( 50 ); //20 times per second - very smooth
   report();
   show();
}

void
ProgressBox::report() //slot
{
   setText( Filelight::ScanManager::files() );
}

void
ProgressBox::stop()
{
   m_timer.stop();
}

void
ProgressBox::halt()
{
   // canceled by stop button
   m_timer.stop();
   QTimer::singleShot( 2000, this, SLOT(hide()) );
}

void
ProgressBox::setText( int files )
{
   QLabel::setText( i18n("%n File", "%n Files", files) );
}

#include "progressBox.moc"
