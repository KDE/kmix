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
#include <kmenubar.h>
#include <ktopwidget.h>
#include "kcontextmenu.h"

#include "sets.h"
#include "mixer.h"
#include "prefs.h"
#include "docking.h"




class KMix : public KTopLevelWidget
{
  Q_OBJECT

public:
  KMix(int mixernum);
  ~KMix();
  Mixer		*mix;
  Preferences	*prefDL;
  DockWidget    *dock_widget;

public slots:
  void onDrop( KDNDDropZone*);
  void showOptsCB();
  void quitClickedCB();
  void launchHelpCB();
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

  bool mainmenuOn;
  bool tickmarksOn;
  bool allowDocking;
  QPopupMenu* contextMenu(QObject *);
  bool eventFilter(QObject *o, QEvent *e);

  void setBalance(int left, int right);
  QWidget	*Container;
  KDNDDropZone  *dropZone;
  KCmManager	*KCM;

  QPopupMenu	*Mfile;
  QPopupMenu	*Moptions;
  QPopupMenu	*Mhelp;
  KMenuBar	*mainmenu;
  QPopupMenu	*Mbalancing;

  QSlider	*LeftRightSB;
  QPoint        KCMpopup_point;

private slots:
  void quit_myapp();
};

#endif
