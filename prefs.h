#ifndef PREFS_H
#define PREFS_H

#include <stdio.h>

#include <qdialog.h>
#include <qpushbt.h>
#include <qchkbox.h>
#include <qbttngrp.h>
#include <qradiobt.h>
#include <qlayout.h>
#include <ktabctl.h>
#include "kconfig.h"
#include "kslider.h"
#include "mixer.h"

class Preferences : public QDialog 
{
      Q_OBJECT
private:
      QPushButton	*buttonOk, *buttonApply, *buttonCancel;
      KTabCtl		*tabctl;
      QWidget		*page1, *page2;
      QGridLayout	*grpbox2a;
public:
      Preferences( QWidget *parent, Mixer *mix );
      void updateChannelConfWindow();
      QCheckBox		*menubarChk;
      QCheckBox		*tickmarksChk;
      KConfig		*kcfg;
      Mixer		*mix;

signals:
	void optionsApply();
	
   public slots:
      void slotShow();
      void slotOk();
      void slotApply();
      void slotCancel();
};


#endif /* PREFS_H */


