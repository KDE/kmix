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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KPREFDLG_H
#define KPREFDLG_H

#include <kdialogbase.h>

class KMixPrefWidget;
class KMixApp;
class QCheckBox;
class QRadioButton;

class 
KMixPrefDlg : public KDialogBase  
{
   Q_OBJECT

   friend class KMixWindow;

  public: 
   KMixPrefDlg( QWidget *parent );
   ~KMixPrefDlg();

  signals:
   void signalApplied( KMixPrefDlg *prefDlg );

   private slots:
      void apply();

  private:
   QFrame *m_generalTab;
   KMixApp *m_mixApp;
   KMixPrefWidget *m_mixPrefTab;

   QCheckBox *m_dockingChk;
   QCheckBox *m_volumeChk;
   QCheckBox *m_hideOnCloseChk;
   QCheckBox *m_showTicks;
   QCheckBox *m_showLabels;
   QCheckBox *m_onLogin;
   QRadioButton *_rbVertical;
   QRadioButton *_rbHorizontal;
   QRadioButton *_rbNone;
   QRadioButton *_rbAbsolute;
   QRadioButton *_rbRelative;
};

#endif
