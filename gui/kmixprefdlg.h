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
class QCheckBox;
class QFrame;
class QRadioButton;

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

  private:
   QFrame *m_generalTab;
   KMixPrefWidget *m_mixPrefTab;

   QCheckBox *m_dockingChk;
   QCheckBox *m_volumeChk;
   QCheckBox *m_showTicks;
   QCheckBox *m_showLabels;
   QCheckBox *m_onLogin;
   QRadioButton *_rbVertical;
   QRadioButton *_rbHorizontal;
};

#endif // KMIXPREFDLG_H
