// -*-C++-*-
#ifndef KMIX_H
#define KMIX_H


// undef Above+Below because of Qt <-> X11 collision. Grr, I hate X11 headers
#undef Above
#undef Below
#include <qslider.h>
#include <qmsgbox.h>
#include <qpopmenu.h>
#include <qmenubar.h>
#include <qtooltip.h>

#include <kurl.h>
#include <kapp.h>
#include <kmsgbox.h>
#include <kmenubar.h>
#include <ktopwidget.h>
#include "kcontextmenu.h"

#include "mixer.h"
#include "prefs.h"




class KMix : public KTopLevelWidget
{
  Q_OBJECT
public:
  KMix(char *mixername);
  ~KMix();
  Mixer		*mix;
  Preferences	*prefDL;
  KConfig	*KmConfig;

public slots:
  void onDrop( KDNDDropZone*);
  void showOptsCB();
  void quitClickedCB();
  void launchHelpCB();
  void aboutClickedCB();
  void aboutqt();
  void applyOptions();

  void MbalCentCB();
  void MbalLeftCB();
  void MbalRightCB();
  void MbalChangeCB(int);

  void placeWidgets();
  void hideMenubarCB();
  void tickmarksTogCB();

private:
  void createWidgets();
  void createMenu();
  void sessionSave();
  void closeEvent( QCloseEvent *e );

  bool mainmenuOn, tickmarksOn;
  QPopupMenu* contextMenu(QObject *);
  bool eventFilter(QObject *o, QEvent *e);

  void setBalance(int left, int right);
  QWidget	*Container;
  KDNDDropZone  *dropZone;
  KCmManager	*KCM;		// <- usually this could be in class Kapp

  QPopupMenu	*Mfile;
  QPopupMenu	*Moptions;
  QPopupMenu	*Mhelp;
  KMenuBar	*mainmenu;
  QPopupMenu	*Mbalancing;

  QSlider	*LeftRightSB;
  QPoint        KCMpopup_point;
};

#endif
