// -*-C++-*-
#ifndef KMIX_H
#define KMIX_H


// undef Above+Below because of Qt <-> X11 collision. Grr, I hate X11 headers
//  #undef Above
//  #undef Below
//  #include <qslider.h>
//  #include <qtimer.h>
//  #include <qpopupmenu.h>
//  #include <qmenubar.h>
//  #include <qtooltip.h>

//  #include <kurl.h>
//  #include <kapp.h>
//  #include <kmenubar.h>
#include <ktmainwindow.h>
#include <dcopobject.h>

//  #include "sets.h"
//  #include "mixer.h"

class QSlider;

class Preferences;
class Mixer;
class KMixDockWidget;

class KMix : public KTMainWindow , DCOPObject
{
  Q_OBJECT
  K_DCOP

public:
  KMix(int mixernum, int SetNum);
  ~KMix();
  bool restore(int n);
  Mixer		*mix;
  Preferences	*prefDL;
  KMixDockWidget    *dock_widget;

k_dcop:
  void activateSet(int l_i_setNum);

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

  void toggleMenubarCB();
  void tickmarksTogCB();
  void updateSliders();

protected:
  void hideEvent( QHideEvent *e );
  void closeEvent( QCloseEvent *e );

signals:
  void newSet( int );
  void updateTicks( bool );

private slots:
  void quit_myapp();
  void quickchange_volume(int diff);
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
  KMenuBar* createMenu();
  void      createWidgets();
  bool  tickmarksOn;
  bool  allowDocking;
  bool  startDocked;
  int	startSet;
  int	startDevice;

  QString	i_s_aboutMsg;
  QPopupMenu* i_m_readSet;
  QPopupMenu* i_m_writeSet;
  QPopupMenu* Mbalancing;
  QPopupMenu* m_defaultPopup;
  QPopupMenu* contextMenu(QObject *);
  QPopupMenu* ContainerContextMenu();
  bool eventFilter(QObject *o, QEvent *e);

  void setBalance( int );

  QSlider	*LeftRightSB;
  QTimer	*i_time;
};

#endif
