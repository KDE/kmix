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
  QCheckBox		*menubarChk;
  QCheckBox		*tickmarksChk;
  QCheckBox		*dockingChk;
  QList<ChannelSetup>	cSetup;
  KConfig		*kcfg;
  Mixer		*mix;

public slots:
  void slotShow();
  void slotOk();
  void slotApply();
  void slotCancel();

signals:
  void optionsApply();

protected:
  void createUserConfWindow();
  void createChannelConfWindow();

private:
  QPushButton	*buttonOk, *buttonApply, *buttonCancel;
  QTabDialog	*tabctl;
  QWidget	*page1, *page2;
  void options2current();
};


#endif /* PREFS_H */


