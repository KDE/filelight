// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#include "debug.h"
#include "deleteDialog.h"
#include <kcursor.h>
#include <kglobal.h>
#include <kio/job.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <qapplication.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qprogressbar.h>


DeleteDialog::DeleteDialog( const KURL &url, bool is_dir, QWidget *parent )
      : QDialog( parent )
      , m_url( url )
      , m_job( 0 )
{
   QString message = is_dir
         ? i18n("The directory and its contents will be <b>recursively</b> and <b>permanently</b> deleted.")
         : i18n("The file will be <b>permanently</b> deleted.");

   message += "<p><a href='#'>";
   message += url.protocol() == "file" ? url.path() : url.prettyURL();

//////
   QLabel *icon = 0;
   KGuiItem const del = KGuiItem( i18n("&Delete"), "editdelete" );

   QBoxLayout *h1 = new QHBoxLayout;
   h1->addWidget( icon = new QLabel( this ) );
   h1->addItem( new QSpacerItem( 6, 6, QSizePolicy::Fixed ) );
   h1->addWidget( new QLabel( message, this ) );

   QBoxLayout *h2 = new QHBoxLayout;
   h2->addWidget( m_bar = new QProgressBar( this ) );
   h2->addItem( new QSpacerItem( 0, 6 ) );
   h2->addWidget( m_ok = new KPushButton( del, this ) );
   h2->addWidget( m_cancel = new KPushButton( KStdGuiItem::cancel(), this ) );

   QBoxLayout *v = new QVBoxLayout( this );
   v->setSpacing( 8 );
   v->setMargin( 8 );
   v->addLayout( h1 );
   v->addItem( new QSpacerItem( 6, 18 ) );
   v->addLayout( h2 );

   m_bar->hide();
   m_bar->setCenterIndicator( true );
   icon->setPixmap( KGlobal::iconLoader()->loadIcon( "messagebox_warning", KIcon::NoGroup, KIcon::SizeLarge, KIcon::DefaultState, 0, true ) );
   icon->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
   m_ok->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
   m_cancel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

   connect( m_cancel, SIGNAL(clicked()), SLOT(reject()) );
   connect( m_ok, SIGNAL(clicked()), SLOT(accept()) );

//////
   setFixedSize( QSize( 4 * m_ok->sizeHint().width(), sizeHint().height() ) );
   setCaption( i18n("Delete Confirmation") );
}

DeleteDialog::~DeleteDialog()
{
   if (m_job) {
      m_job->kill();
      delete m_job;
      QApplication::restoreOverrideCursor();
   }
}

void
DeleteDialog::accept()
{
   Q_DEBUG_BLOCK

   m_job = KIO::del( m_url ); //, false /*shred*/, false /*show fugly progress window*/ );
   //m_job->setWindow( this );

   connect( m_job, SIGNAL(result( KIO::Job* )), SLOT(result( KIO::Job* )) );
   connect( m_job, SIGNAL(percent( KIO::Job*, unsigned long )), SLOT(percent( KIO::Job*, unsigned long )) );

   m_cancel->setText( i18n("&Abort") );
   m_ok->setEnabled( false );
   m_bar->show();

   QApplication::setOverrideCursor( KCursor::workingCursor() );
}

void
DeleteDialog::result( KIO::Job *j )
{
   Q_DEBUG_BLOCK

   hide();

   if (j->error()) {
      j->showErrorDialog( parentWidget() );
      done( IndeterminateResult );
   }
   else
      QDialog::accept();
}

void
DeleteDialog::percent( KIO::Job*, const unsigned long percent )
{
   Q_DEBUG_BLOCK

   m_bar->setProgress( percent );
}
