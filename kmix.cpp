/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-98 Christian Esken
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

// Thanks for taking a look in here. :-)
static char rcsid[]="$Id$";

#include <stdio.h>
#include <unistd.h>
#include <iostream.h>
#include <kapp.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmsgbox.h>
#include <kwm.h>
#include <qmessagebox.h>

#include "sets.h"
#include "mixer.h"
#include "kmix.h"
#include "kmix.moc"
#include "version.h"


#include <qkeycode.h>
#include <qlabel.h>
#include <qaccel.h>


KApplication   *globalKapp;
KIconLoader    *globalKIL;
KMix	       *kmix;
Mixer	       *initMix;
KConfig	       *KmConfig;


signed char	SetNumber;
bool dockinginprogress = false;

extern char	KMixErrors[6][200];

int main(int argc, char **argv)
{
  bool initonly = false;
  int mixer_id  = 0;     // Use default mixer

  SetNumber = -1;
  /* Parse the command line arguments */
  for (int i=0 ; i<argc; i++) {
    if (strcmp(argv[i],"-version") == 0) {
      cout << "kmix " << rcsid << '\n';
      exit(0);
    }
    else if (strcmp(argv[i],"-r") == 0) {
      SetNumber   = 1;
    }
//    else if (strcmp(argv[i],"-init") == 0) {
//      initonly   = true;
//    }
    else if (strcmp(argv[i],"-R") == 0 && i+1<argc) {
      /* -R is the command to read in a specified set.
       * The set number is given as the next argument.
       */
      i=i+1;
      SetNumber   = atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-devnum") == 0  && i+1<argc) {
      mixer_id = atoi(argv[i]);
    }
    else if ( i+1 == argc )
      mixer_id = atoi(argv[i]);
  }


  if (!initonly) {
    // Don't initialize GUI, when we only do "init"
    globalKapp  = new KApplication( argc, argv, "kmix" );
    globalKIL   = globalKapp->getIconLoader();
  }

  if (!initonly && globalKapp->isRestored()) {

    // MODE #1 : Restored by Session Management

    int n = 1;
    while (KTopLevelWidget::canBeRestored(n)) {
      // Read mixer number and set number from session management.
      // This is neccesary, because when the application is restarted
      // by the SM, the
      // should work, too.
      KConfig* scfg = globalKapp->getSessionConfig();
      scfg->setGroup("kmixoptions");
      int startSet    = scfg->readNumEntry("startSet",-1);
      int startDevice = scfg->readNumEntry("startDevice",0);
      kmix = new KMix(startDevice, startSet);
      kmix->restore(n);
      n++;
    }
    return globalKapp->exec();
  }
  else {
    // MODE #2 and #3
    if ( initonly ) {
      // MODE #2 : Only initialize mixer, no GUI
      cout << "Doing initonly ... ";
      initMix = new Mixer( mixer_id, SetNumber );
      cout << "Finished\n";
      return 0;
    }
    else {
      // MODE #3 : Started regulary by the user
      kmix = new KMix( mixer_id, SetNumber );
      return globalKapp->exec();
    }
  }
}

KMix::~KMix()
{
  configSave();
  delete mainmenu;
}

bool KMix::restore(int number)
{
  if (!canBeRestored(number))
    return False;
  KConfig *config = kapp->getSessionConfig();
  if (readPropertiesInternal(config, number)){
    return True;
  }
  return False;

#if 0
  bool ret = KTopLevelWidget::restore(n);
  if (ret && allowDocking && startDocked )
    hide();
  return ret;
#endif
}




KMix::KMix(int mixernum, int SetNum)
{
  // First store how the mixer was started. This two
  // things will be stored in the session config.
  startSet = SetNum;
  startDevice = mixernum;
  KmConfig=KApplication::getKApplication()->getConfig();

  KmConfig->setGroup("");
  mainmenuOn  = KmConfig->readNumEntry( "Menubar"  , 1 );
  tickmarksOn = KmConfig->readNumEntry( "Tickmarks", 1 );
  int Balance;
  Balance     = KmConfig->readNumEntry( "Balance"  , 0 );  // centered by default
  allowDocking= KmConfig->readNumEntry( "Docking"  , 0 );
  startDocked = KmConfig->readNumEntry( "StartDocked"  , 0 );

  KCM = new KCmManager(this);
  CHECK_PTR(KCM);
  mix = new Mixer(mixernum, SetNum);
  CHECK_PTR(mix);

  dock_widget = new DockWidget("dockw");
  if ( allowDocking ) {
    dock_widget->dock();
  }

  connect ( dock_widget, SIGNAL(quit_clicked()), this, SLOT(quit_myapp()  ));
  connect ( globalKapp , SIGNAL(saveYourself()), this, SLOT(sessionSaveAll() ));

  int mixer_error = mix->grab();
  if ( mixer_error != 0 ) {
    KMsgBox::message(0, "Mixer failure.", KMixErrors[mixer_error], KMsgBox::INFORMATION, "OK" );
    mix->errormsg(mixer_error);
    exit(1);
  }


  createWidgets();
  placeWidgets();

  prefDL     = new Preferences(NULL, this->mix);
  prefDL->menubarChk->setChecked  (mainmenuOn );
  prefDL->tickmarksChk->setChecked(tickmarksOn);
  prefDL->dockingChk->setChecked(allowDocking);

  connect(prefDL, SIGNAL(optionsApply()), this, SLOT(applyOptions()));

  globalKapp->setMainWidget( this );
   if ( !allowDocking || !startDocked)
    show();
  else
    hide();
}

void KMix::applyOptions()
{
  mainmenuOn  = prefDL->menubarChk->isChecked();
  tickmarksOn = prefDL->tickmarksChk->isChecked();
  allowDocking= prefDL->dockingChk->isChecked();
  mix->Set0toHW(); // Do NOT write volume after "applying" Options from config dailog
  placeWidgets();
}

void KMix::createWidgets()
{
  QPixmap miniDevPM;
  QPixmap WMminiIcon = globalKIL->loadIcon("mixer_mini.xpm");

  // keep this enum local. It is really only needed here
  enum {audioIcon, bassIcon, cdIcon, extIcon, microphoneIcon,
	midiIcon, recmonIcon,trebleIcon, unknownIcon, volumeIcon };
#ifdef ALSA /* not sure if this is for ALSA in general or just my SB16 */
  char DefaultMixerIcons[]={
    volumeIcon,		bassIcon,	trebleIcon,	midiIcon,	audioIcon,
    extIcon,	microphoneIcon,	cdIcon, recmonIcon, recmonIcon, unknownIcon
  };
  const unsigned char numDefaultMixerIcons=11;
#else
  char DefaultMixerIcons[]={
    volumeIcon,		bassIcon,	trebleIcon,	midiIcon,	audioIcon,
    unknownIcon,	extIcon,	microphoneIcon,	cdIcon,		recmonIcon,
    audioIcon,		recmonIcon,	recmonIcon,	recmonIcon,	extIcon,
    extIcon,		extIcon
  };
  const unsigned char numDefaultMixerIcons=17;
#endif
  // Init DnD: Set up drop zone and drop handler
  dropZone = new KDNDDropZone( this, DndURL );
  connect( dropZone, SIGNAL( dropAction( KDNDDropZone* )),
	   SLOT( onDrop( KDNDDropZone*)));

  // Window title
  setCaption( globalKapp->getCaption() );

  // Create a big container containing every widget of this toplevel
  Container  = new QWidget(this);
  setView(Container);
  //  KCM->insert(Container, (KCmFunc*)contextMenu);  // !!!
  // Create Menu
  createMenu();
  setMenu(mainmenu);
  // Create Sliders (Volume indicators)
  MixDevice *MixPtr = mix->First;
  while (MixPtr) {
    // If you encounter a relayout signal from a mixer device, obey blindly ;-)
    connect((QObject*)MixPtr, SIGNAL(relayout()), this, SLOT(placeWidgets()));

    int devnum = MixPtr->device_num;


    // Figure out default icon
    unsigned char iconnum;
    if (devnum < numDefaultMixerIcons)
      iconnum=DefaultMixerIcons[devnum];
    else
      iconnum=unknownIcon;
    switch (iconnum) {
      // TODO: Should be replaceable by user.
    case audioIcon:
      miniDevPM = globalKIL->loadIcon("mix_audio.xpm");	break;
    case bassIcon:
      miniDevPM = globalKIL->loadIcon("mix_bass.xpm");	break;
    case cdIcon:
      miniDevPM = globalKIL->loadIcon("mix_cd.xpm");	break;
    case extIcon:
      miniDevPM = globalKIL->loadIcon("mix_ext.xpm");	break;
    case microphoneIcon:
      miniDevPM = globalKIL->loadIcon("mix_microphone.xpm");break;
    case midiIcon:
      miniDevPM = globalKIL->loadIcon("mix_midi.xpm");	break;
    case recmonIcon:
      miniDevPM = globalKIL->loadIcon("mix_recmon.xpm");	break;
    case trebleIcon:
      miniDevPM = globalKIL->loadIcon("mix_treble.xpm");	break;
    case unknownIcon:
      miniDevPM = globalKIL->loadIcon("mix_unknown.xpm");	break;
    case volumeIcon:
      miniDevPM = globalKIL->loadIcon("mix_volume.xpm");	break;
    default:
      miniDevPM = globalKIL->loadIcon("mix_unknown.xpm");	break;
    }

    QLabel *qb = new QLabel(Container);
    if (! miniDevPM.isNull())
      qb->setPixmap(miniDevPM);
    else
      cerr << "Pixmap missing.\n";
    MixPtr->picLabel=qb;
    qb->resize(miniDevPM.width(),miniDevPM.height());
    KCM->insert(qb, (KCmFunc*)contextMenu);


    QSlider *VolSB = new QSlider( 0, 100, 10, MixPtr->Left->volume,\
				  QSlider::Vertical, Container, "VolL");

    MixPtr->Left->slider = VolSB;  // Remember the Slider (for the eventFilter)
    connect( VolSB, SIGNAL(valueChanged(int)), MixPtr->Left, SLOT(VolChanged(int)));

    KCM->insert(VolSB, (KCmFunc*)contextMenu);

    // Create a second slider, when the current channel is a stereo channel.
    bool BothSliders = (MixPtr->is_stereo  == true );

    if ( BothSliders) {
      QSlider *VolSB2 = new QSlider( 0, 100, 10, MixPtr->Right->volume,\
				     QSlider::Vertical, Container, "VolR");
      MixPtr->Right->slider= VolSB2;  // Remember Slider (for eventFilter)
      connect( VolSB2, SIGNAL(valueChanged(int)), \
	       MixPtr->Right, SLOT(VolChanged(int)));

      KCM->insert(VolSB2, (KCmFunc*)contextMenu);
    }
    MixPtr=MixPtr->Next;
    // Append MixEntry of current mixer device
  }

  // Create the Left-Right-Slider, add Tooltip and Context menu
  LeftRightSB = new QSlider( -100, 100, 25, 0,\
			     QSlider::Horizontal, Container, "RightLeft");
  connect( LeftRightSB, SIGNAL(valueChanged(int)), \
	   this, SLOT(MbalChangeCB(int)));
  KCM->insert(LeftRightSB, (KCmFunc*)contextMenu);
  QToolTip::add( LeftRightSB, "Left/Right balancing" );
}



void KMix::placeWidgets()
{
  int sliderHeight=100;
  int qsMaxY=0;
  int ix = 0;
  int iy = 0;

  QSlider *qs;
  QLabel  *qb;

  // Place Sliders (Volume indicators)
  ix  = 0;
  if (mainmenuOn)
    mainmenu->show();
  else
    mainmenu->hide();


  bool first = true;
  MixDevice *MixPtr = mix->First;
  while (MixPtr) {
    if (MixPtr->is_disabled) {
      MixPtr->picLabel->hide();
      MixPtr->Left->slider->hide();
      if (MixPtr->is_stereo)
	MixPtr->Right->slider->hide();
      MixPtr=MixPtr->Next;
      continue;
    }

    if ( !first ) ix += 6;
    else          ix += 4; // On first loop add 4

    int old_x=ix;

    qb = MixPtr->picLabel;

    // left slider
    qs = MixPtr->Left->slider;
    if (tickmarksOn) {
      qs->setTickmarks(QSlider::Left);
      qs->setTickInterval(10);
    }
    else
      qs->setTickmarks(QSlider::NoMarks);

    QSize VolSBsize = qs->sizeHint();
    qs->setValue(100-MixPtr->Left->volume);
    qs->setGeometry( ix, iy+qb->height(), VolSBsize.width(), sliderHeight);

    qs->move(ix,iy+qb->height());
    qs->show();

    // Its a good point to find out the maximum y pos of the slider right here
    if (first)
      qsMaxY = qs->y()+qs->height();

    ix += qs->width();

    // But make sure it isn't linked to the left channel.
    bool BothSliders =
      (MixPtr->is_stereo  == true ) &&
      (MixPtr->StereoLink == false);

    QString ToolTipString;
    ToolTipString = MixPtr->name();
    if ( BothSliders)
      ToolTipString += " (Left)";
    QToolTip::add( qs, ToolTipString );

    // Mark record source(s) and muted channel(s). This is done by ordinary
    // color marks on the slider, but this will be changed by red and green
    // and black "bullets" below the slider. TODO !!!
    if (MixPtr->is_recsrc)
      qs->setBackgroundColor( red );
    else {
      if (MixPtr->is_muted)
	qs->setBackgroundColor( black );
      else
	qs->setBackgroundColor( colorGroup().mid() );
    }

    if (MixPtr->is_stereo  == true) {
      qs = MixPtr->Right->slider;
	
      if (MixPtr->StereoLink == false) {
	// Show right slider
	if (tickmarksOn) {
	  qs->setTickmarks(QSlider::Right);
	  qs->setTickInterval(10);
	}
	else
	  qs->setTickmarks(QSlider::NoMarks);
	
	QSize VolSBsize = qs->sizeHint();
	qs->setValue(100-MixPtr->Right->volume);
	qs->setGeometry( ix, iy+qb->height(), VolSBsize.width(), sliderHeight);

	ix += qs->width();
	ToolTipString = MixPtr->name();
	ToolTipString += " (Right)";
	QToolTip::add( qs, ToolTipString );

	if (MixPtr->is_recsrc)
	  qs->setBackgroundColor( red );
	else {
	  if (MixPtr->is_muted)
	    qs->setBackgroundColor( black );
	  else
	    qs->setBackgroundColor( colorGroup().mid() );
	}

	qs->show();
      }
      else
	// Don't show right slider
	qs->hide();
    }

    // Pixmap label. Place it horizontally centered to volume slider(s)
    qb->move((int)((ix + old_x - qb->width() )/2),iy);
    qb->show();


    first=false;
    MixPtr=MixPtr->Next;
  }

  ix += 4;
  iy = qsMaxY;
  LeftRightSB->setGeometry(0,iy,ix,LeftRightSB->sizeHint().height());

  iy+=LeftRightSB->height();
  Container->setFixedSize( ix, iy );
  updateRects();
}


void KMix::slotReadSet1() { slotReadSet(1); }
void KMix::slotReadSet2() { slotReadSet(2); }
void KMix::slotReadSet3() { slotReadSet(3); }
void KMix::slotReadSet4() { slotReadSet(4); }
void KMix::slotWriteSet1() { slotWriteSet(1); }
void KMix::slotWriteSet2() { slotWriteSet(2); }
void KMix::slotWriteSet3() { slotWriteSet(3); }
void KMix::slotWriteSet4() { slotWriteSet(4); }

void KMix::slotReadSet(int num)
{
  mix->Set2Set0(num,true);
  mix->Set0toHW();
  placeWidgets();
}

void KMix::slotWriteSet(int num)
{
  mix->Set0toSet(num);
}

void KMix::createMenu()
{

  QAccel *qAcc = new QAccel( this );

  // Global "Help"-Key
  qAcc->connectItem( qAcc->insertItem(Key_F1),this, SLOT(launchHelpCB()));
  qAcc->connectItem( qAcc->insertItem(Key_1), this,  SLOT(slotReadSet1()));
  qAcc->connectItem( qAcc->insertItem(Key_2), this,  SLOT(slotReadSet2()));
  qAcc->connectItem( qAcc->insertItem(Key_3), this,  SLOT(slotReadSet3()));
  qAcc->connectItem( qAcc->insertItem(Key_4), this,  SLOT(slotReadSet4()));
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_1), this,  SLOT(slotWriteSet1()));
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_2), this,  SLOT(slotWriteSet2()));
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_3), this,  SLOT(slotWriteSet3()));
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_4), this,  SLOT(slotWriteSet4()));




  Mfile = new QPopupMenu;
  CHECK_PTR( Mfile );
  Mfile->insertItem(i18n("&Hide Menubar")    , this, SLOT(hideMenubarCB()) , CTRL+Key_M);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_M),this, SLOT(hideMenubarCB()));

  Mfile->insertItem(i18n("&Tickmarks On/Off"), this, SLOT(tickmarksTogCB()), CTRL+Key_T);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_T),this, SLOT(tickmarksTogCB()));

  Mfile->insertItem( i18n("&Options...")       , this, SLOT(showOptsCB()) );
  Mfile->insertSeparator();
  Mfile->insertItem( i18n("E&xit")          , this, SLOT(quitClickedCB()) , CTRL+Key_Q);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_Q),this, SLOT(quitClickedCB()));

  QString msg,head;

  msg  = "KMix ";
  msg += APP_VERSION;
  msg += i18n("\n(C) 1997-1998 by Christian Esken (esken@kde.org).\n\n" \
    "Sound mixer panel for the KDE Desktop Environment.\n"\
    "This program is in the GPL.\n"\
    "SGI Port done by Paul Kendall (paul@orion.co.nz).\n"\
    "*BSD fixes by Sebestyen Zoltan (szoli@digo.inf.elte.hu)\n"\
    "and Lennart Augustsson (augustss@cs.chalmers.se).\n"\
    "ALSA port by Nick Lopez (kimo_sabe@usa.net).");
  head += APP_VERSION;

  Mhelp = globalKapp->getHelpMenu(true,msg);
  CHECK_PTR( Mhelp );

  mainmenu = new KMenuBar( this, "main menu");
  CHECK_PTR( mainmenu );
  mainmenu->insertItem( i18n("&File"), Mfile );
  mainmenu->insertSeparator();
  mainmenu->insertItem( i18n("&Help"), Mhelp );

  Mbalancing = new QPopupMenu;
  CHECK_PTR( Mbalancing );
  Mbalancing->insertItem(i18n("&Left")  , this, SLOT(MbalLeftCB()));
  Mbalancing->insertItem(i18n("&Center"), this, SLOT(MbalCentCB()));
  Mbalancing->insertItem(i18n("&Right") , this, SLOT(MbalRightCB()));
}

void KMix::tickmarksTogCB()
{
  tickmarksOn=!tickmarksOn;
  placeWidgets();
}

void KMix::hideMenubarCB()
{
  mainmenuOn=!mainmenuOn;
  placeWidgets();
}

void KMix::showOptsCB()
{
  prefDL->show();
}

void KMix::quitClickedCB()
{
  quit_myapp();
}



void KMix::launchHelpCB()
{
  globalKapp->invokeHTMLHelp("kmix/index.html", "");
}


bool KMix::event(QEvent *e)
{
  if (e->type() == Event_Hide && allowDocking && !dockinginprogress) {
    sleep(1); // give kwm some time..... ugly I know.
    if (!KWM::isIconified(winId())) // maybe we are just on another desktop
      return FALSE;

    if(dock_widget)
      dock_widget->dock();
    this->hide();
    // a trick to remove the window from the taskbar (Matthias)
    recreate(0,0, QPoint(x(), y()), FALSE);
    globalKapp->setTopWidget( this );
    return TRUE;
  }
  return QWidget::event(e);
}

bool KMix::eventFilter(QObject *o, QEvent *e)
{
  // Lets see, if we have a "Right mouse button press"
  if (e->type() == Event_MouseButtonPress) {
    QMouseEvent *qme = (QMouseEvent*)e;
    if (qme->button() == RightButton) {
      QPopupMenu *qpm = contextMenu(o,0);

      if (qpm) {
	KCMpopup_point = QCursor::pos();
	qpm->popup(KCMpopup_point);
	return true;
      }
    }
  }
  return false;
}


QPopupMenu* KMix::ContainerContextMenu(QObject *o, QObject *)
{
  static bool MlocalCreated=false;
  static QPopupMenu *Mlocal;

  if (MlocalCreated)
    delete Mlocal;

  Mlocal = new QPopupMenu;
  if ( mainmenuOn )
    Mlocal->insertItem( i18n("&Hide Menubar") , this, SLOT(hideMenubarCB()) );
  else
    Mlocal->insertItem( i18n("&Show Menubar") , this, SLOT(hideMenubarCB()) );
  Mlocal->insertItem( i18n("&Options...")        , this, SLOT(showOptsCB()) );
  Mlocal->insertItem( i18n("&Help")           , this, SLOT(launchHelpCB()) );

  MlocalCreated = true;
  return Mlocal;
}



QPopupMenu* KMix::contextMenu(QObject *o, QObject *e)
{
  if ( o == LeftRightSB )
    return Mbalancing;

  static bool MlocalCreated=false;
  static QPopupMenu *Mlocal;

  if (o == NULL)
    return NULL;

  if (MlocalCreated)
    delete Mlocal;

  // Scan mixerChannels for Slider object *o
  MixDevice     *MixPtr = mix->First;
  QSlider       *qs     = (QSlider*)o;
  MixDevice     *MixFound= NULL;
  while(MixPtr) {
    if ( (MixPtr->Left->slider == qs) || (MixPtr->Right->slider == qs) ) {
      MixFound = MixPtr;
      break;
    }
    MixPtr = MixPtr->Next;
  }

  // Have not found slider => return and do not pop up context menu
  if ( MixFound == NULL )
    return ContainerContextMenu(o,e);  // Default context menu


  // else
  Mlocal = new QPopupMenu;
  CHECK_PTR( Mlocal );
  if (MixFound->is_muted)
    Mlocal->insertItem(i18n("Un&mute")    , MixPtr, SLOT(MvolMuteCB()    ));
  else
    Mlocal->insertItem(i18n("&Mute")      , MixPtr, SLOT(MvolMuteCB()    ));
  if (MixFound->is_stereo) {
    if (MixFound->StereoLink)
      Mlocal->insertItem(i18n("&Split")   , MixPtr, SLOT(MvolSplitCB()   ));
    else
      Mlocal->insertItem(i18n("Un&split") , MixPtr, SLOT(MvolSplitCB()   ));
  }
  if (MixFound->is_recordable)
    Mlocal->insertItem(i18n("&RecSource") , MixPtr, SLOT(MvolRecsrcCB()  ));

  MlocalCreated = true;
  return Mlocal;
}





void KMix::onDrop( KDNDDropZone* _zone )
{
  QStrList strlist;
  KURL *url;

  strlist = _zone->getURLList();
  url = new KURL( strlist.first() );
  delete url;
}


void KMix::MbalCentCB()
{
  setBalance(100,100);
}

void KMix::MbalLeftCB()
{
  setBalance(100,0);
}

void KMix::MbalRightCB()
{
  setBalance(0,100);
}

void KMix::MbalChangeCB(int pos)
{
  if ( pos < 0 )
    setBalance(100,100+pos);
  else
    setBalance(100-pos,100);
}



void KMix::setBalance(int left, int right)
{
  mix->setBalance(left,right);
  if (left==100)
    LeftRightSB->setValue(right-100);
  else
    LeftRightSB->setValue(100-left);
}


// Session management and config saving


void KMix::sessionSaveAll()
{
  sessionSave(true);
}
void KMix::configSave()
{
  sessionSave(false);
}

void KMix::sessionSave(bool sessionConfig)
{
  KmConfig->setGroup("");
  KmConfig->writeEntry( "Balance"    , LeftRightSB->value() , true );
  KmConfig->writeEntry( "Menubar"    , mainmenuOn  );
  KmConfig->writeEntry( "Tickmarks"  , tickmarksOn );
  KmConfig->writeEntry( "Docking"    , allowDocking);
  bool iv = isVisible();
  KmConfig->writeEntry( "StartDocked", !iv);

  if (sessionConfig) {
    // Save session specific data only when needed
    KConfig* scfg = globalKapp->getSessionConfig();
    scfg->setGroup("kmixoptions");
    scfg->writeEntry("startSet", startSet);
    scfg->writeEntry("startDevice", startDevice);
  }
  mix->sessionSave(sessionConfig);
  KmConfig->sync();
}

void KMix::closeEvent( QCloseEvent *e )
{
    dockinginprogress = true;
    configSave();
    KTopLevelWidget::closeEvent(e);
    /*
    if ( allowDocking ) {
        dock_widget->dock();
        this->hide();
    }else
    KTopLevelWidget::closeEvent(e);
    */
}

void KMix::quit_myapp()
{
  configSave();
  globalKapp->quit();
}
