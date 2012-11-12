/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KMIXPREFDLG_H
#define KMIXPREFDLG_H

#include <kdialog.h>

class KMixPrefWidget;

class QBoxLayout;
class QCheckBox;
class QFrame;
class QLabel;
class QRadioButton;
class QShowEvent;
class QWidget;

class 
KMixPrefDlg : public KDialog
{
   Q_OBJECT

   friend class KMixWindow;

  public: 
   KMixPrefDlg( QWidget *parent );
   virtual ~KMixPrefDlg();

  signals:
   void signalApplied( KMixPrefDlg *prefDlg );

   private slots:
      void apply();
      void dockIntoPanelChange(int state);

  protected:
    void showEvent ( QShowEvent * event );

  private:
    void addWidgetToLayout(QWidget* widget, QBoxLayout* layout, int spacingBefore, QString toopTipText);


    QFrame *m_generalTab;

   QCheckBox *m_dockingChk;
   QCheckBox *m_volumeChk;
   QLabel *dynamicControlsRestoreWarning;
   QCheckBox *m_showTicks;
   QCheckBox *m_showLabels;
   QCheckBox* m_showOSD;
   QCheckBox *m_onLogin;
   QCheckBox *allowAutostart;
   QLabel *allowAutostartWarning;
   QCheckBox *m_beepOnVolumeChange;
   QLabel *volumeFeedbackWarning;
   QRadioButton *_rbVertical;
   QRadioButton *_rbHorizontal;
   QRadioButton *_rbTraypopupVertical;
   QRadioButton *_rbTraypopupHorizontal;
};

#endif // KMIXPREFDLG_H
