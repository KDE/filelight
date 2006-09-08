// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#ifndef DELETE_DIALOG_H
#define DELETE_DIALOG_H

#include <qdialog.h>
#include <kurl.h>
namespace KIO { class Job; }
class QProgressBar;
class QPushButton;


class DeleteDialog : public QDialog
{
   Q_OBJECT

   KURL const m_url;
   KIO::Job *m_job;

   QProgressBar *m_bar;
   QPushButton *m_cancel, *m_ok;

   virtual void accept();

public:
   DeleteDialog( const KURL&, bool is_dir, QWidget *parent );
   ~DeleteDialog();

   enum Result { IndeterminateResult = 1000 };

private slots:
   /// I'm sorry, but KIO signals lack elegance, so great job there
   void percent( KIO::Job *job, unsigned long percent );
   void result( KIO::Job *job );
};

#endif
