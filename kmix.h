// -*-C++-*-
#ifndef KMIX_H
#define KMIX_H


// undef Above+Below because of Qt <-> X11 collision. Grr, I hate X11 headers
#undef Above
#undef Below
#include <qslider.h>
#include <qtimer.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qtooltip.h>

#include <kurl.h>
#include <kapp.h>
#include <kmenubar.h>
#include <ktmainwindow.h>
#include <dcopobject.h>

#include "sets.h"
#include "mixer.h"
#include "prefs.h"
#include "kmix-docking.h"

class KMix : public KTMainWindow , DCOPObject
{
  Q_OBJECT
  K_DCOP

public:
  // Constructs a KMix object, defined by the number (platform dependent). The given set is loaded.
  KMix(int mixernum, int SetNum);
  // Destructs KMix
  ~KMix();
  bool restore(int n);
  Mixer			*i_mixer;
  Preferences		*prefDL;
  KMixDockWidget	*dock_widget;

public slots:
  void showOptsCB();
  void quitClickedCB();
  void launchHelpCB();
  void launchAboutCB();
  void applyOptions();

  void MbalCentCB();
  void MbalLeftCB();
  void MbalRightCB();
  void MbalChangeCB(int);

  void placeWidgets();
  void hideMenubarCB();
  void tickmarksTogCB();
  void updateSliders();
  void updateSlidersI();

signals:
  void layoutChange();
  
protected:
  void hideEvent( QHideEvent *e );
  void closeEvent( QCloseEvent *e );


private slots:
  void quit_myapp();
  void quickchange_volume(int val_l_diff);
  void sessionSaveAll();
  void configSave();
  void sessionSave(bool sessionConfig);
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
  bool mainmenuOn;
  bool tickmarksOn;
  bool allowDocking;
  bool startDocked; 
  int		startSet;
  int		startDevice;

  QString	i_s_aboutMsg;

  QPopupMenu* contextMenu(QObject *, QObject *);
  QPopupMenu* ContainerContextMenu(QObject *, QObject *);


  bool eventFilter(QObject *o, QEvent *e);
  void setBalance(int left, int right);

  QWidget	*i_widget_container;

  KMenuBar	*i_menu_main;

  QPopupMenu	*i_popup_balancing;
  QPopupMenu	*i_popup_readSet;
  QPopupMenu	*i_popup_writeSet;


  QLabel	*i_lbl_infoLine;
  QLabel	*i_lbl_setNum;

  QSlider	*i_slider_leftRight;
  QPoint        i_point_popup;
  QTimer	*i_time;
};

#endif
