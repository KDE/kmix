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

// Thanks for taking a look in here. :-)
static char rcsid[]="$Id$";

#include <stdio.h>
#include <unistd.h>
#include <iostream.h>

#include <kapp.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kwm.h>

#include <kmessagebox.h>

#include "sets.h"
#include "mixer.h"
#include "kmix.h"
#include "kmix.moc"
#include "version.h"


#include <qkeycode.h>
#include <qlabel.h>
#include <qaccel.h>
#include <qmessagebox.h>
#include <qtooltip.h>

KApplication   *globalKapp;
KMix	       *kmix;
Mixer	       *initMix;
KConfig	       *KmConfig;


signed char	SetNumber;
bool dockinginprogress = false;

int main(int argc, char **argv)
{
  bool initonly = false;
  int mixer_id  = 0;     // Use default mixer

  SetNumber = -1;
  /* Parse the command line arguments */
  for (int i=1 ; i<argc; i++) {
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
      i += 1;
      SetNumber   = atoi(argv[i]);
    }
    else if (strcmp(argv[i],"-devnum") == 0  && i+1<argc) {
      i += 1;
      mixer_id = atoi(argv[i]);
    }
    else if ( i+1 == argc )
      mixer_id = atoi(argv[i]);
  }


  if (!initonly) {
    // Don't initialize GUI, when we only do "init"
    globalKapp  = new KApplication( argc, argv, "kmix" );
  }

  if (!initonly && globalKapp->isRestored()) {

    // MODE #1 : Restored by Session Management

    int n = 1;
    while (KTMainWindow::canBeRestored(n)) {
      // Read mixer number and set number from session management.
      // This is neccesary, because when the application is restarted
      // by the SM, the
      // should work, too.
      KConfig* scfg = globalKapp->sessionConfig();
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
      initMix = Mixer::getMixer( mixer_id, SetNumber );
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

  if (  i_time != 0 ) delete i_time;
}

bool KMix::restore(int number)
{
  if (!canBeRestored(number))
    return False;
  KConfig *config = kapp->sessionConfig();
  if (readPropertiesInternal(config, number)){
    return True;
  }
  return False;

#if 0
  bool ret = KTMainWindow::restore(n);
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

  i_time = 0;
  KmConfig=KApplication::kApplication()->config();

  KmConfig->setGroup(0);
  mainmenuOn  = KmConfig->readNumEntry( "Menubar"  , 1 );
  tickmarksOn = KmConfig->readNumEntry( "Tickmarks", 1 );
  int Balance;
  Balance     = KmConfig->readNumEntry( "Balance"  , 0 );  // centered by default
  allowDocking= KmConfig->readNumEntry( "Docking"  , 0 );
  startDocked = KmConfig->readNumEntry( "StartDocked"  , 0 );

  mix = Mixer::getMixer(mixernum, SetNum);
  CHECK_PTR(mix);

  dock_widget = new KMixDockWidget((QString)"dockw", "kmixdocked");
  dock_widget->setMainWindow(this);
  if ( allowDocking ) {
    dock_widget->dock();
  }

  connect ( dock_widget, SIGNAL(quit_clicked()), this, SLOT(quit_myapp()  ));
  connect ( dock_widget, SIGNAL(quickchange(int)), this, SLOT(quickchange_volume(int)  ));
  connect ( globalKapp , SIGNAL(saveYourself()), this, SLOT(sessionSaveAll() ));

  int mixer_error = mix->grab();
  if ( mixer_error != 0 ) {
    KMessageBox::error(0, mix->errorText(mixer_error), i18n("Mixer failure"));
    mix->errormsg(mixer_error);
    exit(1);
  }


  createWidgets();
  if (SetNum > 0) {
    i_lbl_setNum->setText( QString(" %1 ").arg(SetNum));
  }
  placeWidgets();

  prefDL     = new Preferences(NULL, this->mix);
  prefDL->menubarChk->setChecked  (mainmenuOn );
  prefDL->tickmarksChk->setChecked(tickmarksOn);
  prefDL->dockingChk->setChecked(allowDocking);

  connect(prefDL, SIGNAL(optionsApply()), this, SLOT(applyOptions()));

  globalKapp->setMainWidget( this );
   if ( allowDocking && startDocked)
    hide();
  else
    show();
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
  bool i_b_first = true;

  QPixmap miniDevPM;

  QPixmap WMminiIcon = BarIcon("mixer_mini");

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

  // Window title
  setCaption( mix->mixerName() );

  // Create a big container containing every widget of this toplevel
  Container  = new QWidget(this);
  setView(Container);
  // Create Menu
  createMenu();
  setMenu(mainmenu);

  // Create the info line
  i_lbl_infoLine = new QLabel(Container) ;
  i_lbl_infoLine->setText(mix->mixerName());
  QFont f10("Helvetica", 10, QFont::Normal);
  i_lbl_infoLine->setFont( f10 );
  i_lbl_infoLine->resize(i_lbl_infoLine->sizeHint());
  //  i_lbl_infoLine->setAlignment(QLabel::AlignRight);
  QToolTip::add( i_lbl_infoLine, mix->mixerName() );

  i_lbl_setNum =  new QLabel(Container);
  i_lbl_setNum->setText("   "); // set a dummy Text, so that the height() is valid.
  QFont f8("Helvetica", 10, QFont::Bold);
  i_lbl_setNum->setFont( f8 );
  i_lbl_setNum->setBackgroundMode(PaletteLight);
  i_lbl_setNum->resize( i_lbl_setNum->sizeHint());
  QToolTip::add( i_lbl_setNum, i18n("Shows the current set number"));

  // Create Sliders (Volume indicators)
  MixDevice *MixPtr = mix->First;
  while (MixPtr) {
    // If you encounter a relayout signal from a mixer device, obey blindly ;-)
    connect((QObject*)MixPtr, SIGNAL(relayout()), this, SLOT(placeWidgets()));

    int devnum = MixPtr->num();


    // Figure out default icon
    unsigned char iconnum;
    if (devnum < numDefaultMixerIcons)
      iconnum=DefaultMixerIcons[devnum];
    else
      iconnum=unknownIcon;
    switch (iconnum) {
      // TODO: Should be replaceable by user.
    case audioIcon:
      miniDevPM = BarIcon("mix_audio");	break;
    case bassIcon:
      miniDevPM = BarIcon("mix_bass");	break;
    case cdIcon:
      miniDevPM = BarIcon("mix_cd");	break;
    case extIcon:
      miniDevPM = BarIcon("mix_ext");	break;
    case microphoneIcon:
      miniDevPM = BarIcon("mix_microphone");break;
    case midiIcon:
      miniDevPM = BarIcon("mix_midi");	break;
    case recmonIcon:
      miniDevPM = BarIcon("mix_recmon");	break;
    case trebleIcon:
      miniDevPM = BarIcon("mix_treble");	break;
    case unknownIcon:
      miniDevPM = BarIcon("mix_unknown");	break;
    case volumeIcon:
      miniDevPM = BarIcon("mix_volume");	break;
    default:
      miniDevPM = BarIcon("mix_unknown");	break;
    }

    QLabel *qb = new QLabel(Container);
    if (! miniDevPM.isNull()) {
      qb->setPixmap(miniDevPM);
      qb->installEventFilter(this);
    } 
    else {
      cerr << "Pixmap missing.\n";
    }
    MixPtr->picLabel=qb;
    qb->resize(miniDevPM.width(),miniDevPM.height());


    // Create state LED
    MixPtr->i_KLed_state = new QceStateLED(Container);

    // Create slider
    QSlider *VolSB = new QSlider( 0, 100, 10, MixPtr->Left->volume,\
				  QSlider::Vertical, Container, "VolL");
    if (i_b_first) {
      VolSB->setFocus();
    }

    MixPtr->Left->slider = VolSB;  // Remember the Slider (for the eventFilter)
    connect( VolSB, SIGNAL(valueChanged(int)), MixPtr->Left, SLOT(VolChanged(int)));
    VolSB->installEventFilter(this);


    // Create a second slider, when the current channel is a stereo channel.
    bool l_b_bothSliders;
    l_b_bothSliders = (MixPtr->stereo()  == true );

    if ( l_b_bothSliders) {
      QSlider *VolSB2 = new QSlider( 0, 100, 10, MixPtr->Right->volume,\
				     QSlider::Vertical, Container, "VolR");
      MixPtr->Right->slider= VolSB2;  // Remember Slider (for eventFilter)
      connect( VolSB2, SIGNAL(valueChanged(int)), MixPtr->Right, SLOT(VolChanged(int)));
      VolSB2->installEventFilter(this);

    }

    i_b_first = false;
    MixPtr=MixPtr->Next;
    // Append MixEntry of current mixer device
  }

  // Create the Left-Right-Slider, add Tooltip and Context menu
  LeftRightSB = new QSlider( -100, 100, 25, 0,\
			     QSlider::Horizontal, Container, "RightLeft");
  connect( LeftRightSB, SIGNAL(valueChanged(int)), \
	   this, SLOT(MbalChangeCB(int)));
  LeftRightSB->installEventFilter(this);
  QToolTip::add( LeftRightSB, "Left/Right balancing" );

  i_time = new QTimer();
  connect( i_time,     SIGNAL(timeout()),      SLOT(updateSliders()) );
  i_time->start( 20 );
}





void KMix::placeWidgets()
{
  int sliderHeight=100;
  int qsMaxY=0;
  int l_i_belowSlider=0;
  int ix = 4;
  int iy = 0;

  QSlider *qs;
  QLabel  *qb;
  QceStateLED	  *l_KLed_state;

  // Place Sliders (Volume indicators)
  if (mainmenuOn)
    mainmenu->show();
  else
    mainmenu->hide();

  iy = i_lbl_setNum->height();

  bool first = true;
  MixDevice *MixPtr = mix->First;
  while (MixPtr) {


    if (MixPtr->disabled() ) {
      // Volume regulator not shown => Hide complete and skip the rest of the code
      MixPtr->picLabel->hide();
      MixPtr->Left->slider->hide();
      if (MixPtr->stereo())
	MixPtr->Right->slider->hide();
      MixPtr->i_KLed_state->hide();
      MixPtr=MixPtr->Next;
      continue;
    }

    // Add some blank space between sliders
    if ( !first ) ix += 6;

    int old_x=ix;

    qb = MixPtr->picLabel;

    // Tickmarks
    qs = MixPtr->Left->slider;
    if (tickmarksOn) {
      qs->setTickmarks(QSlider::Right);
      qs->setTickInterval(10);
    }
    else
      qs->setTickmarks(QSlider::NoMarks);

    QSize VolSBsize = qs->sizeHint();
    qs->setValue(100-MixPtr->Left->volume);


    qs->setGeometry( ix, iy+qb->height(), VolSBsize.width(), sliderHeight);
    qs->show();


    // Its a good point to find out the maximum y pos of the slider right here
    if (first) {
      l_i_belowSlider = iy+qb->height() + sliderHeight + 4;
     }

    ix += qs->width();

    // But make sure it isn't linked to the left channel.
    bool l_b_bothSliders =
      (MixPtr->stereo()       == true ) &&
      (MixPtr->stereoLinked() == false);

    QString ToolTipString;
    ToolTipString = MixPtr->name();
    if ( l_b_bothSliders)
      ToolTipString += " (Left)";
    QToolTip::add( qs, ToolTipString );

    // Mark record source(s) and muted channel(s). This is done by using
    // red and green and 'off' LED's
    l_KLed_state = MixPtr->i_KLed_state;
    if (MixPtr->muted()) {
      // Is muted => Off
      l_KLed_state->setState( QceStateLED::Off );
      l_KLed_state->setColor( Qt::black );
    }
    else {
      if (MixPtr->recsrc()) {
	// Is record source => Red
	l_KLed_state->setState( QceStateLED::On );
	l_KLed_state->setColor(  Qt::red );
      }
      else {
	// Is in standard mode (playback) => Green
	l_KLed_state->setState( QceStateLED::On );
	l_KLed_state->setColor( Qt::green );
      }
    }
    l_KLed_state->show();

    if (MixPtr->stereo()  == true) {
      qs = MixPtr->Right->slider;
	
      if (MixPtr->stereoLinked() == false) {
	// Show right slider
	if (tickmarksOn) {
	  qs->setTickmarks(QSlider::Left);
	  qs->setTickInterval(10);
	}
	else {
	  qs->setTickmarks(QSlider::NoMarks);
	}
	QSize VolSBsize = qs->sizeHint();
	qs->setValue(100-MixPtr->Right->volume);
	qs->setGeometry( ix, iy+qb->height(), VolSBsize.width(), sliderHeight);

	ix += qs->width();
	ToolTipString = MixPtr->name();
	ToolTipString += " (Right)";
	QToolTip::add( qs, ToolTipString );

	qs->show();
      }
      else {
	// Don't show right slider
	qs->hide();
      }
    }


    // Pixmap label. Place it horizontally centered to volume slider(s)
    qb->move((int)((ix + old_x - qb->width() )/2),iy);
    qb->show();


    // The same for the state LED
    int l_i_newWidth;
    l_i_newWidth = ix - old_x - 6;
    
    int l_i_xpos, l_i_height;
    l_i_height = l_KLed_state->height();

    l_i_xpos  = ix + old_x;
    l_i_xpos -= l_i_newWidth;
    l_i_xpos /= 2;
 
    l_KLed_state->setGeometry(l_i_xpos,l_i_belowSlider, l_i_newWidth, l_i_height);
    l_KLed_state->show();

    if (first) {
      qsMaxY = l_i_belowSlider + l_i_height;
    }

    first=false;
    MixPtr=MixPtr->Next;
  }

  ix += 4;
  iy = qsMaxY +4;
  LeftRightSB->setGeometry(0,iy,ix,LeftRightSB->sizeHint().height());


  // Size the set number.
  i_lbl_setNum->resize( i_lbl_setNum->sizeHint());
  // Now I know how many space the set number needs.
  // The rest is going to the infoLine Label
  QSize l_qsz_infoWidth = i_lbl_infoLine->sizeHint();

  int l_i_allowedWidth = ix - i_lbl_setNum->width();
  if ( l_i_allowedWidth < 1) {
    l_i_allowedWidth = 1;
  }
  if ( l_i_allowedWidth > l_qsz_infoWidth.width() ) {
    l_i_allowedWidth = l_qsz_infoWidth.width();
  }
  i_lbl_infoLine->resize(l_i_allowedWidth, i_lbl_infoLine->height());
  i_lbl_infoLine->move(ix - i_lbl_infoLine->width(),0);

  iy+=LeftRightSB->height();
  Container->setFixedSize( ix, iy );

  // tell the Toplevel to do a relayout
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
  i_lbl_setNum->setText( QString(" %1 ").arg(num));
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

  QString head;

  i_s_aboutMsg  = "KMix ";
  i_s_aboutMsg += APP_VERSION;
  i_s_aboutMsg += i18n("\n(C) 1997-2000 by Christian Esken (esken@kde.org).\n\n" \
    "Sound mixer panel for the KDE Desktop Environment.\n"\
    "This program is in the GPL.\n"\
    "SGI Port done by Paul Kendall (paul@orion.co.nz).\n"\
    "*BSD fixes by Sebestyen Zoltan (szoli@digo.inf.elte.hu)\n"\
    "and Lennart Augustsson (augustss@cs.chalmers.se).\n"\
    "ALSA port by Nick Lopez (kimo_sabe@usa.net).");
  head += APP_VERSION;

#if QT_VERSION >= 200
  Mhelp = helpMenu(i_s_aboutMsg);
#else
  Mhelp = globalKapp->helpMenu(true, i_s_aboutMsg);
#endif

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
  globalKapp->invokeHTMLHelp("", "");
}

void KMix::launchAboutCB()
{
  QMessageBox::about( 0L, globalKapp->caption(), i_s_aboutMsg ); 
}


bool KMix::eventFilter(QObject *o, QEvent *e)
{
  // Lets see, if we have a "Right mouse button press"
  if (e->type() == QEvent::MouseButtonPress)
 {
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


QPopupMenu* KMix::ContainerContextMenu(QObject *, QObject *)
{
  static bool MlocalCreated=false;
  static QPopupMenu *Mlocal;

  if (MlocalCreated) {
    MlocalCreated = false;
    delete Mlocal;
  }
  Mlocal = new QPopupMenu;
  if ( mainmenuOn )
    Mlocal->insertItem( i18n("&Hide Menubar") , this, SLOT(hideMenubarCB()) );
  else
    Mlocal->insertItem( i18n("&Show Menubar") , this, SLOT(hideMenubarCB()) );
  Mlocal->insertItem( i18n("&Options...")        , this, SLOT(showOptsCB()) );
  Mlocal->insertSeparator();
  Mlocal->insertItem( i18n("&About")           , this, SLOT(launchAboutCB()) );
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

  if (MlocalCreated) {
    MlocalCreated = false;
    delete Mlocal;
  }

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
  if (MixFound->muted())
    Mlocal->insertItem(i18n("Un&mute")    , MixPtr, SLOT(MvolMuteCB()    ));
  else
    Mlocal->insertItem(i18n("&Mute")      , MixPtr, SLOT(MvolMuteCB()    ));
  if (MixFound->stereo()) {
    if (MixFound->stereoLinked())
      Mlocal->insertItem(i18n("&Split")   , MixPtr, SLOT(MvolSplitCB()   ));
    else
      Mlocal->insertItem(i18n("Un&split") , MixPtr, SLOT(MvolSplitCB()   ));
  }
  if (MixFound->recordable())
    Mlocal->insertItem(i18n("&RecSource") , MixPtr, SLOT(MvolRecsrcCB()  ));

  MlocalCreated = true;
  return Mlocal;
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
  KmConfig->setGroup(0);
  KmConfig->writeEntry( "Balance"    , LeftRightSB->value() , true );
  KmConfig->writeEntry( "Menubar"    , mainmenuOn  );
  KmConfig->writeEntry( "Tickmarks"  , tickmarksOn );
  KmConfig->writeEntry( "Docking"    , allowDocking);
  bool iv = isVisible();
  KmConfig->writeEntry( "StartDocked", !iv);

  if (sessionConfig) {
    // Save session specific data only when needed
    KConfig* scfg = globalKapp->sessionConfig();
    scfg->setGroup("kmixoptions");
    scfg->writeEntry("startSet", startSet);
    scfg->writeEntry("startDevice", startDevice);
  }
  mix->sessionSave(sessionConfig);
  KmConfig->sync();
}

void KMix::closeEvent( QCloseEvent *e )
{
  cout << "closeEvent()\n";
    dockinginprogress = true;
    configSave();
    KTMainWindow::closeEvent(e);
    /*
    if ( allowDocking ) {
        dock_widget->dock();
        this->hide();
    }else
    KTMainWindow::closeEvent(e);
    */
}


void KMix::hideEvent( QHideEvent *)
{
  cout << "hideEvent()\n";
  if ( allowDocking && !dockinginprogress) {
    if(dock_widget)
      dock_widget->dock();
    this->hide();
    // a trick to remove the window from the taskbar (Matthias)
    recreate(0,0, QPoint(x(), y()), FALSE);
    globalKapp->setTopWidget( this );
    return ;
  }
}


void KMix::updateSliders( )
{
  mix->Set2Set0(-1,true);        // Read from hardware
  updateSlidersI();
}

void KMix::updateSlidersI( )
{
  QSlider *qs;
  MixSet *SrcSet = mix->TheMixSets->first();

  // now update the slider positions...
  MixDevice        *MixPtr = mix->First;

  bool setDisplay = true;

  /* The next line is tricky. Lets explain it:
     Several lines later I will call qs->setValue(). Doing so changes the value of the QSlider.

     This will lead to the emitting of its valueChanged() signal. This again ist the hint for
     my code that the user has dragged the slider. As the user expects that the volume changes
     when he drags the slider, this action is indeed being triggered.

     The above scenario shows that everything goes well. Alas - it wasn't the user who dragged
     the slider. We only want to display changes that other programs did - so we are NOT expected
     to write to the hardware again. Using the HW_update(false) does exactly this (avoiding
     writing to the hardware).
  */
  MixChannel::HW_update(false);

  while(MixPtr) {
    MixSetEntry *mse;

    // Traverse all MixSetEntries to find the entry which corresponds to  MixPtr->num()
    for(mse = SrcSet->first();
	(mse != NULL) && (mse->devnum != MixPtr->num() );
	mse=SrcSet->next() );
    if (mse == NULL)
      continue;		// -<- not found : Shouldn't this better break the loop?   !!!		(1)

    if(! MixPtr->disabled()){
      MixPtr->Left->volume = mse->volumeL;
      MixPtr->Right->volume = mse->volumeR;
    }
    qs = MixPtr->Left->slider;
    qs->setValue(100-MixPtr->Left->volume);
    if (MixPtr->stereo()  == true) {
      qs = MixPtr->Right->slider;
      qs->setValue(100-MixPtr->Right->volume);
    }
    if ( setDisplay) {
      dock_widget->setDisplay(( MixPtr->Left->volume + MixPtr->Right->volume )/2);
      setDisplay = false;
    }


    MixPtr = MixPtr->Next;	// !!! Not executed in the case "mse == NULL", see at (1)
  }
  MixChannel::HW_update(true);
}



void KMix::quit_myapp()
{
  configSave();
  globalKapp->quit();
}

void KMix::quickchange_volume(int val_l_diff)
{
  int l_i_volNew;
  //cerr << "quickchange diff = " << val_l_diff;


  MixDevice        *MixPtr = mix->First;

  MixSet *Set0  = mix->TheMixSets->first();
  MixSetEntry *mse = Set0->findDev(MixPtr->num() );
  if (mse) {
    // left volume
    l_i_volNew =  mse->volumeL + val_l_diff;
    if (l_i_volNew > 100) l_i_volNew = 100;
    if (l_i_volNew <   0) l_i_volNew = 0;
    MixPtr->Left->VolChangedI(l_i_volNew);
    if (! MixPtr->stereoLinked() ) {
      // right volume
      l_i_volNew =  mse->volumeR + val_l_diff;
      if (l_i_volNew > 100) l_i_volNew = 100;
      if (l_i_volNew <   0) l_i_volNew = 0;
      MixPtr->Right->VolChangedI(l_i_volNew);
    }
    updateSliders();
  }

}
