//-*-C++-*-
/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken
 *                        esken@kde.org
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

#ifndef PREFS_H
#define PREFS_H

#include <stdio.h>


#include <qcheckbox.h>

#include <qtabdialog.h>
#include "kconfig.h"
#include "mixer.h"
#include "channel.h"



/// Preferences dialog for kmix
class Preferences : public QTabDialog
{
      Q_OBJECT

public:
   Preferences( QWidget *parent, Mixer *mix );
  ~Preferences(void) { };
  void createOptionsConfWindow(QWidget *);
  void createChannelConfWindow(QWidget *);
  QCheckBox		*menubarChk;
  QCheckBox		*tickmarksChk;
  QCheckBox		*dockingChk;
  QList<ChannelSetup>	cSetup;
  KConfig		*kcfg;
  Mixer		*mix;


private:
  QPushButton	*buttonOk, *buttonApply, *buttonCancel;
  QTabDialog	*tabctl;
  QWidget	*page1, *page2;

  void options2current();

signals:
      void optionsApply();
	
public slots:
      void slotShow();
      void slotOk();
      void slotApply();
      void slotCancel();
};


#endif /* PREFS_H */


