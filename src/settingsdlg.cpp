/***************************************************************************
                          settingsdlg.cpp  -  description
                             -------------------
    begin                : Wed Jul 30 2003
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

#include <qapplication.h> //Getting desktop width
#include <qslider.h>
#include <qpushbutton.h>
#include <qlistbox.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <qvbox.h>
#include <qcheckbox.h>
#include <qtimer.h>

#include <kdirselectdialog.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kdebug.h>


#include "settingsdlg.h"
#include "settings.h"



#define TIMEOUT 1000


SettingsDlg::SettingsDlg( Settings *s, QWidget *parent, const char *name )
 : SettingsDialog( parent, name, false ), //3rd param => modal
   m_settings( s ),
   m_timer( new QTimer( this ) )
{
  colourSchemeGroup->setFrameShape( QFrame::NoFrame );

  colourSchemeGroup->insert( new QRadioButton( "Rainbow", colourSchemeGroup ), scheme_rainbow );
  colourSchemeGroup->insert( new QRadioButton( "KDE Colours", colourSchemeGroup ), scheme_kde );
  colourSchemeGroup->insert( new QRadioButton( "High Contrast", colourSchemeGroup ), scheme_highContrast );

  //read in settings before you make all those nasty connections!
  reset(); //makes gui reflect global settings

  connect( m_timer, SIGNAL( timeout() ), this, SLOT( timeout() ) );

  connect( m_addButton,    SIGNAL( clicked() ), this, SLOT( addDirectory() ) );
  connect( m_removeButton, SIGNAL( clicked() ), this, SLOT( removeDirectory() ) );
  connect( m_resetButton,  SIGNAL( clicked() ), this, SLOT( reset() ) );
  connect( m_closeButton,  SIGNAL( clicked() ), this, SLOT( close() ) );

  connect( colourSchemeGroup, SIGNAL( clicked( int ) ), this, SLOT( changeScheme( int ) ) );
  connect( contrastSlider, SIGNAL( valueChanged( int ) ), this, SLOT( changeContrast( int ) ) );
  connect( contrastSlider, SIGNAL( sliderReleased() ), this, SLOT( slotSliderReleased() ) );
  
  connect( scanAcrossMounts,       SIGNAL( toggled( bool ) ), this, SLOT( startTimer() ) );
  connect( dontScanRemoteMounts,   SIGNAL( toggled( bool ) ), this, SLOT( startTimer() ) );
  connect( dontScanRemovableMedia, SIGNAL( toggled( bool ) ), this, SLOT( startTimer() ) );

  connect( useAntialiasing,    SIGNAL( toggled( bool ) ), this, SLOT( toggleUseAntialiasing( bool ) ) );
  connect( varyLabelFontSizes, SIGNAL( toggled( bool ) ), this, SLOT( toggleVaryLabelFontSizes( bool ) ) );
  connect( showSmallFiles,     SIGNAL( toggled( bool ) ), this, SLOT( toggleShowSmallFiles( bool ) ) );  

  connect( minFontPitch, SIGNAL ( valueChanged( int ) ), this, SLOT( changeMinFontPitch( int ) ) );

  m_addButton->setIconSet( SmallIcon( "folder_side" ) );
}


SettingsDlg::~SettingsDlg()
{
}


void SettingsDlg::closeEvent( QCloseEvent * e )
{
  //if an invalidation is pending, force it now!
  if( m_timer->isActive() )
    m_timer->changeInterval( 0 );

  m_settings->writeSettings();

  QWidget::closeEvent( e );

  deleteLater();
}


void SettingsDlg::reset()
{
  m_settings->readSettings();

  //tab 1
  scanAcrossMounts->setChecked( m_settings->scanAcrossMounts );
  dontScanRemoteMounts->setChecked( !m_settings->scanRemoteMounts );
  dontScanRemovableMedia->setChecked( !m_settings->scanRemovableMedia );

  dontScanRemoteMounts->setEnabled( m_settings->scanAcrossMounts );
//  dontScanRemovableMedia.setEnabled( m_settings->scanAcrossMounts );

  m_listBox->clear();
  m_listBox->insertStringList( m_settings->skipList );
  m_listBox->setSelected( 0, true );

  if( m_listBox->count() == 0 )
    m_removeButton->setEnabled( false );
  else
    m_removeButton->setEnabled( true );

  //tab 2
  if( colourSchemeGroup->id( colourSchemeGroup->selected() ) != m_settings->scheme )
  {
    colourSchemeGroup->setButton( m_settings->scheme );
      //setButton doesn't call a single QButtonGroup signal!
      //so we need to call this ourselves (and hence the detection above)
      changeScheme( m_settings->scheme );
  }
  contrastSlider->setValue( m_settings->contrast );

  useAntialiasing->setChecked( (m_settings->aaFactor > 1) ? true : false );

  varyLabelFontSizes->setChecked( m_settings->varyLabelFontSizes );
  minFontPitch->setEnabled( m_settings->varyLabelFontSizes );
  minFontPitch->setValue( m_settings->minFontPitch );
  showSmallFiles->setChecked( m_settings->showSmallFiles );
}



void SettingsDlg::toggleScanAcrossMounts( bool b )
{
  m_settings->scanAcrossMounts = b;

  dontScanRemoteMounts->setEnabled( b );
//  dontScanRemovableMedia.setEnabled( b );
}

void SettingsDlg::toggleDontScanRemoteMounts( bool b )
{
  m_settings->scanRemoteMounts = !b;
}

void SettingsDlg::toggleDontScanRemovableMedia( bool b )
{
  m_settings->scanRemovableMedia = !b;
}



void SettingsDlg::addDirectory()
{
  KURL url = KDirSelectDialog::selectDirectory( "/", false, this, "Scan Directory" );

  //**** error handling!
  //**** wrong protocol handling!

  QString path = url.path( 1 );

  if( m_settings->skipList.contains( path ) )
    KMessageBox::sorry( this, "That directory is already set to be excluded from scans" );

  else {
    m_settings->skipList.append( path );
    m_listBox->insertItem( path );
    m_removeButton->setEnabled( true );
  }
}


void SettingsDlg::removeDirectory()
{
  //**** really is bad to have to independent lists representing the same thing.
  //**** would be better to handle both lists with indices. should be foolproof
  //  ** if you just ensure the UI looks to be working this should work fine, hopefully (was problem with if same dir being removed from skipList then it would break UI )

  m_settings->skipList.remove( m_listBox->currentText() ); //removes all similar entries, which is probably what user wants?

  //safest method to ensure consistency
  m_listBox->clear();
  m_listBox->insertStringList( m_settings->skipList );

  if( m_listBox->count() == 0 ) m_removeButton->setEnabled( false );
}


void SettingsDlg::startTimer()
{
  m_timer->start( TIMEOUT, true );
}


void SettingsDlg::timeout()
{
  m_timer->stop();

  emit mapIsInvalid(); //will empty cache and force a rescan
}


void SettingsDlg::changeScheme( int s )
{
  m_settings->scheme = (MapScheme)s;
  emit canvasIsDirty( 1 );
}
void SettingsDlg::changeContrast( int c )
{
  m_settings->contrast = c;
  emit canvasIsDirty( 3 );
}
void SettingsDlg::toggleUseAntialiasing( bool b )
{
  m_settings->aaFactor = b ? 2 : 1;
  emit canvasIsDirty( 2 );
}
void SettingsDlg::toggleVaryLabelFontSizes( bool b )
{
  m_settings->varyLabelFontSizes = b;
  minFontPitch->setEnabled( b );
  emit canvasIsDirty( 0 );
}
void SettingsDlg::changeMinFontPitch( int p )
{
  m_settings->minFontPitch = p;
  emit canvasIsDirty( 0 );
}
void SettingsDlg::toggleShowSmallFiles( bool b )
{
  m_settings->showSmallFiles = b;
  emit canvasIsDirty( 1 );
}


void SettingsDlg::slotSliderReleased()
{
  emit canvasIsDirty( 2 );
}


void SettingsDlg::reject()
{
  //called when escape is pressed
  reset();
  QDialog::reject();   //**** doesn't change back scheme so far
}

#include "settingsdlg.moc"
