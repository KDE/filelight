/***************************************************************************
                          settingsdlg.h  -  description
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

#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include "settingsdialog.h" //generate by uic



class QWidget;
class QTimer;
class QRect;
 
class SettingsDlg : public SettingsDialog
{

Q_OBJECT

public: 
  SettingsDlg( QWidget *parent=0, const char *name=0 );
  virtual ~SettingsDlg();

protected:
  virtual void closeEvent( QCloseEvent * e );
  virtual void reject();

public slots:
  void addDirectory();
  void removeDirectory();
  void toggleScanAcrossMounts( bool );
  void toggleDontScanRemoteMounts( bool );
  void toggleDontScanRemovableMedia( bool );
  void reset();
  void startTimer();
  void timeout();
  void toggleUseAntialiasing( bool = true );
  void toggleVaryLabelFontSizes( bool );
  void changeContrast( int );
  void changeScheme( int );
  void changeMinFontPitch( int );
  void toggleShowSmallFiles( bool );
  void slotSliderReleased();
      
signals:
  void mapIsInvalid();
  void canvasIsDirty( int );

private:
  QTimer *m_timer;
};

#endif
