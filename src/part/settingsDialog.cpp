//Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-4
//Copyright: See COPYING file that comes with this distribution

#include <qapplication.h> //Getting desktop width
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qvbuttongroup.h>

#include <kdirselectdialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>

#include "settingsDialog.h"
#include "Config.h"


SettingsDialog::SettingsDialog( QWidget *parent, const char *name )
  : Dialog( parent, name, false ) //3rd param => modal
{
    colourSchemeGroup->setFrameShape( QFrame::NoFrame );

    colourSchemeGroup->insert( new QRadioButton( i18n("Rainbow"), colourSchemeGroup ), Filelight::Rainbow );
    colourSchemeGroup->insert( new QRadioButton( i18n("KDE Colors"), colourSchemeGroup ), Filelight::KDE );
    colourSchemeGroup->insert( new QRadioButton( i18n("High Contrast"), colourSchemeGroup ), Filelight::HighContrast );

    //read in settings before you make all those nasty connections!
    reset(); //makes dialog reflect global settings

    connect( &m_timer, SIGNAL(timeout()), SIGNAL(mapIsInvalid()) );

    connect( m_addButton,    SIGNAL( clicked() ), SLOT( addDirectory() ) );
    connect( m_removeButton, SIGNAL( clicked() ), SLOT( removeDirectory() ) );
    connect( m_resetButton,  SIGNAL( clicked() ), SLOT( reset() ) );
    connect( m_closeButton,  SIGNAL( clicked() ), SLOT( close() ) );

    connect( colourSchemeGroup, SIGNAL(clicked( int )), SLOT(changeScheme( int )) );
    connect( contrastSlider, SIGNAL(valueChanged( int )), SLOT(changeContrast( int )) );
    connect( contrastSlider, SIGNAL(sliderReleased()), SLOT(slotSliderReleased()) );

    connect( scanAcrossMounts,       SIGNAL( toggled( bool ) ), SLOT( startTimer() ) );
    connect( dontScanRemoteMounts,   SIGNAL( toggled( bool ) ), SLOT( startTimer() ) );
    connect( dontScanRemovableMedia, SIGNAL( toggled( bool ) ), SLOT( startTimer() ) );

    connect( useAntialiasing,    SIGNAL( toggled( bool ) ), SLOT( toggleUseAntialiasing( bool ) ) );
    connect( varyLabelFontSizes, SIGNAL( toggled( bool ) ), SLOT( toggleVaryLabelFontSizes( bool ) ) );
    connect( showSmallFiles,     SIGNAL( toggled( bool ) ), SLOT( toggleShowSmallFiles( bool ) ) );

    connect( minFontPitch, SIGNAL ( valueChanged( int ) ), SLOT( changeMinFontPitch( int ) ) );

    m_addButton->setIconSet( SmallIcon( "fileopen" ) );
    m_resetButton->setIconSet( SmallIcon( "undo" ) );
    m_closeButton->setIconSet( SmallIcon( "fileclose" ) );
}


void SettingsDialog::closeEvent( QCloseEvent* )
{
    //if an invalidation is pending, force it now!
    if( m_timer.isActive() ) m_timer.changeInterval( 0 );

    Config::write();

    deleteLater();
}


void SettingsDialog::reset()
{
    Config::read();

    //tab 1
    scanAcrossMounts->setChecked( Config::scanAcrossMounts );
    dontScanRemoteMounts->setChecked( !Config::scanRemoteMounts );
    dontScanRemovableMedia->setChecked( !Config::scanRemovableMedia );

    dontScanRemoteMounts->setEnabled( Config::scanAcrossMounts );
    //  dontScanRemovableMedia.setEnabled( Config::scanAcrossMounts );

    m_listBox->clear();
    m_listBox->insertStringList( Config::skipList );
    m_listBox->setSelected( 0, true );

    m_removeButton->setEnabled( m_listBox->count() == 0 );

    //tab 2
    if( colourSchemeGroup->id( colourSchemeGroup->selected() ) != Config::scheme )
    {
        colourSchemeGroup->setButton( Config::scheme );
        //setButton doesn't call a single QButtonGroup signal!
        //so we need to call this ourselves (and hence the detection above)
        changeScheme( Config::scheme );
    }
    contrastSlider->setValue( Config::contrast );

    useAntialiasing->setChecked( (Config::antiAliasFactor > 1) ? true : false );

    varyLabelFontSizes->setChecked( Config::varyLabelFontSizes );
    minFontPitch->setEnabled( Config::varyLabelFontSizes );
    minFontPitch->setValue( Config::minFontPitch );
    showSmallFiles->setChecked( Config::showSmallFiles );
}



void SettingsDialog::toggleScanAcrossMounts( bool b )
{
    Config::scanAcrossMounts = b;

    dontScanRemoteMounts->setEnabled( b );
    //dontScanRemovableMedia.setEnabled( b );
}

void SettingsDialog::toggleDontScanRemoteMounts( bool b )
{
    Config::scanRemoteMounts = !b;
}

void SettingsDialog::toggleDontScanRemovableMedia( bool b )
{
    Config::scanRemovableMedia = !b;
}



void SettingsDialog::addDirectory()
{
    const KURL url = KDirSelectDialog::selectDirectory( "/", false, this );

    //TODO error handling!
    //TODO wrong protocol handling!

    if( !url.isEmpty() )
    {
        const QString path = url.path( 1 );

        if( !Config::skipList.contains( path ) )
        {
            Config::skipList.append( path );
            m_listBox->insertItem( path );
            m_removeButton->setEnabled( true );
        }
        else KMessageBox::sorry( this, i18n("That directory is already set to be excluded from scans") );
    }
}


void SettingsDialog::removeDirectory()
{
    Config::skipList.remove( m_listBox->currentText() ); //removes all entries that match

    //safest method to ensure consistency
    m_listBox->clear();
    m_listBox->insertStringList( Config::skipList );

    m_removeButton->setEnabled( m_listBox->count() == 0 );
}


void SettingsDialog::startTimer()
{
    m_timer.start( TIMEOUT, true );
}

void SettingsDialog::changeScheme( int s )
{
    Config::scheme = (Filelight::MapScheme)s;
    emit canvasIsDirty( 1 );
}
void SettingsDialog::changeContrast( int c )
{
    Config::contrast = c;
    emit canvasIsDirty( 3 );
}
void SettingsDialog::toggleUseAntialiasing( bool b )
{
    Config::antiAliasFactor = b ? 2 : 1;
    emit canvasIsDirty( 2 );
}
void SettingsDialog::toggleVaryLabelFontSizes( bool b )
{
    Config::varyLabelFontSizes = b;
    minFontPitch->setEnabled( b );
    emit canvasIsDirty( 0 );
}
void SettingsDialog::changeMinFontPitch( int p )
{
    Config::minFontPitch = p;
    emit canvasIsDirty( 0 );
}
void SettingsDialog::toggleShowSmallFiles( bool b )
{
    Config::showSmallFiles = b;
    emit canvasIsDirty( 1 );
}


void SettingsDialog::slotSliderReleased()
{
    emit canvasIsDirty( 2 );
}


void SettingsDialog::reject()
{
    //called when escape is pressed
    reset();
    QDialog::reject();   //**** doesn't change back scheme so far
}

#include "settingsDialog.moc"
