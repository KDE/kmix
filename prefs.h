//-*-C++-*-
#ifndef PREFS_H
#define PREFS_H

#include <stdio.h>


#include <qchkbox.h>

#include <qtabdlg.h>
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
  void createChannelConfWindow();
  QCheckBox		*menubarChk;
  QCheckBox		*tickmarksChk;
  QCheckBox		*dockingChk;
  QList<ChannelSetup>	cSetup;
  KConfig		*kcfg;
  Mixer		*mix;


private:
  QPushButton	*buttonOk, *buttonApply, *buttonCancel;
  QTabDialog	*tabctl;
  QWidget		*page1, *page2;
  QGroupBox		*grpbox2a;

signals:
      void optionsApply();
	
public slots:
      void slotShow();
      void slotOk();
      void slotApply();
      void slotCancel();
};


#endif /* PREFS_H */


