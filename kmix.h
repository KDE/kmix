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
  bool restore(int n);
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

private slots:
  void sessionSave();
  void slotReadSet1();
  void slotReadSet2();
  void slotReadSet3();
  void slotReadSet4();
  void slotWriteSet1();
  void slotWriteSet2();
  void slotWriteSet3();
  void slotWriteSet4();

  void slotReadSet(int num);
  void slotWriteSet(int num);

private:
  void createWidgets();
  void createMenu();
  void closeEvent( QCloseEvent *e );

  bool mainmenuOn;
  bool tickmarksOn;
  bool allowDocking;
  bool startDocked; 
  QPopupMenu* contextMenu(QObject *, QObject *);
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
