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


#include "settingsdlg.h"
#include "settings.h"
#include "scanthread.h"



#define TIMEOUT 1000

extern Settings Gsettings;


SettingsDlg::SettingsDlg( QWidget *parent, const char *name ) : SettingsDialog( parent, name, false ) //3rd param => modal
{
  m_timer = new QTimer( this );

  colourSchemeGroup->setFrameShape( QFrame::NoFrame );

  colourSchemeGroup->insert( new QRadioButton( "Rainbow", colourSchemeGroup ), scheme_rainbow );
  colourSchemeGroup->insert( new QRadioButton( "KDE Colours", colourSchemeGroup ), scheme_kde );
  colourSchemeGroup->insert( new QRadioButton( "High Contrast", colourSchemeGroup ), scheme_highContrast );
//  colourSchemeGroup->insert( new QRadioButton( "File Density", colourSchemeGroup ), scheme_fileDensity );

  //read in settings before you make all those nasty connections!
  reset(); //reads global Settings and makes this dialog's gui reflect them
  
  connect( m_timer, SIGNAL( timeout() ), this, SLOT( timeout() ) );

  connect( m_addButton,       SIGNAL( clicked() ), this, SLOT( addDirectory() ) );
  connect( m_removeButton,    SIGNAL( clicked() ), this, SLOT( removeDirectory() ) );
  connect( m_resetButton,     SIGNAL( clicked() ), this, SLOT( reset() ) );
  connect( m_closeButton,     SIGNAL( clicked() ), this, SLOT( close() ) );

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
  //if a rescan is pending, force it now!
  if( m_timer->isActive() )
    m_timer->changeInterval( 0 );

  Gsettings.writeSettings();

  QWidget::closeEvent( e );
}


void SettingsDlg::reset()
{
  Gsettings.readSettings();

  //tab 1
  scanAcrossMounts->setChecked( Gsettings.scanAcrossMounts );
  dontScanRemoteMounts->setChecked( !Gsettings.scanRemoteMounts );
  dontScanRemovableMedia->setChecked( !Gsettings.scanRemovableMedia );

  dontScanRemoteMounts->setEnabled( Gsettings.scanAcrossMounts );
//  dontScanRemovableMedia.setEnabled( Gsettings.scanAcrossMounts );

  m_listBox->clear();
  m_listBox->insertStringList( Gsettings.skipList );
  m_listBox->setSelected( 0, true );

  if( m_listBox->count() == 0 )
    m_removeButton->setEnabled( false );
  else
    m_removeButton->setEnabled( true );

  //tab 2
  if( colourSchemeGroup->id( colourSchemeGroup->selected() ) != Gsettings.scheme )
  {
    colourSchemeGroup->setButton( Gsettings.scheme );
      //setButton doesn't call a single QButtonGroup signal!
      //so we need to call this ourselves (and hence the detection above)
      changeScheme( Gsettings.scheme );
  }
  contrastSlider->setValue( Gsettings.contrast );

  useAntialiasing->setChecked( (Gsettings.aaFactor > 1) ? true : false );

  varyLabelFontSizes->setChecked( Gsettings.varyLabelFontSizes );
  minFontPitch->setEnabled( Gsettings.varyLabelFontSizes );
  minFontPitch->setValue( Gsettings.minFontPitch );
  showSmallFiles->setChecked( Gsettings.showSmallFiles );
}



void SettingsDlg::toggleScanAcrossMounts( bool b )
{
  Gsettings.scanAcrossMounts = b;

  dontScanRemoteMounts->setEnabled( b );
//  dontScanRemovableMedia.setEnabled( b );
}

void SettingsDlg::toggleDontScanRemoteMounts( bool b )
{
  Gsettings.scanRemoteMounts = !b;
}

void SettingsDlg::toggleDontScanRemovableMedia( bool b )
{
  Gsettings.scanRemovableMedia = !b;
}



void SettingsDlg::addDirectory()
{
  KURL url = KDirSelectDialog::selectDirectory( "/", false, this, "Scan Directory" );

  //**** error handling!
  //**** wrong protocol handling!

  QString path = url.path( 1 );

  if( Gsettings.skipList.contains( path ) )
    KMessageBox::sorry( this, "That directory is already set to be excluded from scans" );

  else {
    Gsettings.skipList.append( path );
    m_listBox->insertItem( path );
    m_removeButton->setEnabled( true );
  }
}


void SettingsDlg::removeDirectory()
{
  //**** really is bad to have to independent lists representing the same thing.
  //**** would be better to handle both lists with indices. should be foolproof
  //  ** if you just ensure the UI looks to be working this should work fine, hopefully (was problem with if same dir being removed from skipList then it would break UI )

  Gsettings.skipList.remove( m_listBox->currentText() ); //removes all similar entries, which is probably what user wants?

  //safest method to ensure consistency
  m_listBox->clear();
  m_listBox->insertStringList( Gsettings.skipList );

  if( m_listBox->count() == 0 ) m_removeButton->setEnabled( false );
}


void SettingsDlg::startTimer()
{
  m_timer->start( TIMEOUT, true );
}


void SettingsDlg::timeout()
{
  m_timer->stop();

  emit treeCacheInvalidated(); //will empty cache and force a rescan
}


void SettingsDlg::changeScheme( int s )
{
  Gsettings.scheme = (MapScheme)s;
  emit dirtyCanvas( 1 );
}
void SettingsDlg::changeContrast( int c )
{
  Gsettings.contrast = c;
  emit dirtyCanvas( 3 );
}
void SettingsDlg::toggleUseAntialiasing( bool b )
{
  Gsettings.aaFactor = b ? 2 : 1;
  emit dirtyCanvas( 2 );
}
void SettingsDlg::toggleVaryLabelFontSizes( bool b )
{
  Gsettings.varyLabelFontSizes = b;
  minFontPitch->setEnabled( b );
  emit dirtyCanvas( 0 );
}
void SettingsDlg::changeMinFontPitch( int p )
{
  Gsettings.minFontPitch = p;
  emit dirtyCanvas( 0 );
}
void SettingsDlg::toggleShowSmallFiles( bool b )
{
  Gsettings.showSmallFiles = b;
  emit dirtyCanvas( 1 );
}


void SettingsDlg::slotSliderReleased()
{
  emit dirtyCanvas( 2 );
}


void SettingsDlg::reject()
{
  //called when escape is pressed
  reset();
  QDialog::reject();   //**** doesn't change back scheme so far
}

#include "settingsdlg.moc"
