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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef KPREFDLG_H
#define KPREFDLG_H

#include <qtabdialog.h>

class KMixPrefWidget;
class KMixApp;
class QCheckBox;

class KMixPrefDlg : public QTabDialog  {
   Q_OBJECT

   friend class KMixWindow;

  public: 
   KMixPrefDlg();
   ~KMixPrefDlg();

  signals:
   void signalApplied( KMixPrefDlg *prefDlg );

   private slots:
      void apply();

  private:
   QWidget *m_generalTab;
   KMixApp *m_mixApp;
   KMixPrefWidget *m_mixPrefTab;

   QCheckBox *m_dockingChk;
   QCheckBox *m_volumeChk;
   QCheckBox *m_hideOnCloseChk;
   QCheckBox *m_showTicks;
   QCheckBox *m_showLabels;
};

#endif
