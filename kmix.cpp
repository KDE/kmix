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
#include <dcopclient.h>

#include <kmessagebox.h>
#include <kstddirs.h>

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
    // Only initialize GUI, when we only do not just "init"
    globalKapp  = new KApplication( argc, argv, "kmix" );
    // setup dcop communication
    if ( !kapp->dcopClient()->isAttached() )
      kapp->dcopClient()->registerAs("kmix");
  }

  //(  KGlobal::dirs()->addResourceType("mini", KStandardDirs::kde_default("data") +     "kmix/pics");
    
  KGlobal::dirs()->addResourceType("icon", KStandardDirs::kde_default("data") + "kmix/pics");


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
  delete i_menu_main;

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




KMix::KMix(int mixernum, int SetNum) : DCOPObject("KMix")
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

  i_mixer = Mixer::getMixer(mixernum, SetNum);

  dock_widget = new KMixDockWidget((QString)"dockw", "kmixdocked");
  dock_widget->setMainWindow(this);
  if ( allowDocking ) {
    dock_widget->dock();
  }

  connect ( dock_widget, SIGNAL(quit_clicked()), this, SLOT(quit_myapp()  ));
  connect ( dock_widget, SIGNAL(quickchange(int)), this, SLOT(quickchange_volume(int)  ));
  connect ( globalKapp , SIGNAL(saveYourself()), this, SLOT(sessionSaveAll() ));

  int mixer_error = i_mixer->grab();
  if ( mixer_error != 0 ) {
    KMessageBox::error(0, i_mixer->errorText(mixer_error), i18n("Mixer failure"));
    i_mixer->errormsg(mixer_error);
    exit(1);
  }


  createWidgets();
  if (SetNum > 0) {
    i_lbl_setNum->setText( QString(" %1 ").arg(SetNum));
  }
  placeWidgets();

  prefDL     = new Preferences(NULL, this->i_mixer);
  prefDL->menubarChk->setChecked  (mainmenuOn );
  prefDL->tickmarksChk->setChecked(tickmarksOn);
  prefDL->dockingChk->setChecked(allowDocking);

  // Synchronize from KTMW to Prefs and vice versa via signals
  connect(prefDL, SIGNAL(optionsApply()), this  , SLOT(applyOptions()));
  connect(this,   SIGNAL(layoutChange()), prefDL, SLOT(slotUpdatelayout()));

  //showOptsCB();  // !!! For faster debugging

  globalKapp->setMainWidget( this );
   if ( allowDocking && startDocked)
    hide();
  else
    show();
}

bool KMix::process(const QCString &fun, const QByteArray &data,
		   QCString& /*replyType*/, QByteArray& /*replyData*/ )
{
  if ( fun == "activateSet(int)" ) {
    QDataStream dataStream( data, IO_ReadOnly );
    int l_i_setNum;
    dataStream >> l_i_setNum;
    switch( l_i_setNum ) {
    case 1:
      slotReadSet1(); break;
    case 2:
      slotReadSet2(); break;
    case 3:
      slotReadSet3(); break;
    case 4:
      slotReadSet4(); break;
    }
    return TRUE;
  }
  return FALSE;
}


void KMix::applyOptions()
{
  mainmenuOn  = prefDL->menubarChk->isChecked();
  tickmarksOn = prefDL->tickmarksChk->isChecked();
  allowDocking= prefDL->dockingChk->isChecked();
#warning Why is here no Set2Set0 ???
  // !!!  i_mixer->Set0toHW(); // Do NOT write volume after "applying" Options from config dailog
  placeWidgets();
}

void KMix::createWidgets()
{
  bool i_b_first = true;

  QPixmap miniDevPM;

  QPixmap WMminiIcon = BarIcon("mini-kmix");

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
  setCaption( i_mixer->mixerName() );

  // Create a big container containing every widget of this toplevel
  i_widget_container  = new QWidget(this);

  setView(i_widget_container);
  // Create Menu
  createMenu();
  setMenu(i_menu_main);


  // Create the info line
  i_lbl_infoLine = new QLabel(i_widget_container) ;
  i_lbl_infoLine->setText(i_mixer->mixerName());
//    QFont f10("Helvetica", 10, QFont::Normal);
//    i_lbl_infoLine->setFont( f10 );
  i_lbl_infoLine->resize(i_lbl_infoLine->sizeHint());
  //  i_lbl_infoLine->setAlignment(QLabel::AlignRight);
  QToolTip::add( i_lbl_infoLine, i_mixer->mixerName() );

  i_lbl_setNum =  new QLabel(i_widget_container);
  i_lbl_setNum->setText("   "); // set a dummy Text, so that the height() is valid.
//    QFont f8("Helvetica", 10, QFont::Bold);
//    i_lbl_setNum->setFont( f8 );
  i_lbl_setNum->setBackgroundMode(PaletteLight);
  i_lbl_setNum->resize( i_lbl_setNum->sizeHint());
  QToolTip::add( i_lbl_setNum, i18n("Shows the current set number"));


  // Create Sliders (Volume indicators)
  for ( unsigned int l_i_mixDevice = 0; l_i_mixDevice < i_mixer->size(); l_i_mixDevice++) {
    MixDevice &MixPtr = (*i_mixer)[l_i_mixDevice];

    // If you encounter a relayout signal from a mixer device, obey blindly ;-)
    // #warning This might be called multiple times (e.g. on a set change). I should change it
    // OK, it doesn't happen. And I know why. But still I might want to rework this
    connect( &MixPtr, SIGNAL(relayout()), this, SLOT(placeWidgets()));

    int devnum = MixPtr.num();


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

    QLabel *qb = new QLabel(i_widget_container);
    if (! miniDevPM.isNull()) {
      qb->setPixmap(miniDevPM);
      qb->installEventFilter(this);
    }
    else {
      cerr << "Pixmap missing.\n";
    }
    MixPtr.picLabel=qb;
    qb->resize(miniDevPM.width(),miniDevPM.height());


    // Create state LED
    MixPtr.i_KLed_state = new QceStateLED(i_widget_container);

    // Create slider
    QSlider *VolSB = new QSlider( 0, 100, 10, MixPtr.volume(0),\
				  QSlider::Vertical, i_widget_container, "VolL");
    if (i_b_first) {
      VolSB->setFocus();
    }

    MixPtr.Left->slider = VolSB;  // Remember the Slider (for the eventFilter)
    connect( VolSB, SIGNAL(valueChanged(int)), MixPtr.Left, SLOT(VolChanged(int)));
    VolSB->installEventFilter(this);


    // Create a second slider, when the current channel is a stereo channel.
    bool l_b_bothSliders;
    l_b_bothSliders = (MixPtr.stereo()  == true );

    if ( l_b_bothSliders) {
      QSlider *VolSB2 = new QSlider( 0, 100, 10, MixPtr.volume(1),\
				     QSlider::Vertical, i_widget_container, "VolR");
      MixPtr.Right->slider= VolSB2;  // Remember Slider (for eventFilter)
      connect( VolSB2, SIGNAL(valueChanged(int)), MixPtr.Right, SLOT(VolChanged(int)));
      VolSB2->installEventFilter(this);

    }

    i_b_first = false;
    // Append MixEntry of current mixer device
  }

  // Create the Left-Right-Slider, add Tooltip and Context menu
  i_slider_leftRight = new QSlider( -100, 100, 25, 0,\
			     QSlider::Horizontal, i_widget_container, "RightLeft");
  connect( i_slider_leftRight, SIGNAL(valueChanged(int)), \
	   this, SLOT(MbalChangeCB(int)));
  i_slider_leftRight->installEventFilter(this);
  QToolTip::add( i_slider_leftRight, "Left/Right balancing" );

  i_time = new QTimer();
  connect( i_time,     SIGNAL(timeout()),      SLOT(updateSliders()) );
  i_time->start( 1000 );

}





void KMix::placeWidgets()
{
  i_widget_container->setUpdatesEnabled(false);
  // !!!debug("Placing widgets");
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
    i_menu_main->show();
  else
    i_menu_main->hide();

  iy = i_lbl_setNum->height();

  bool first = true;

  for ( unsigned int l_i_mixDevice = 0; l_i_mixDevice < i_mixer->size(); l_i_mixDevice++) {
    MixDevice &MixPtr = (*i_mixer)[l_i_mixDevice];

    if (MixPtr.disabled() ) {
      // Volume regulator not shown => Hide complete and skip the rest of the code
      MixPtr.picLabel->hide();
      MixPtr.Left->slider->hide();
      if (MixPtr.stereo())
	MixPtr.Right->slider->hide();
      MixPtr.i_KLed_state->hide();
      continue;
    }

    // Add some blank space between sliders
    if ( !first ) ix += 6;

    int old_x=ix;

    qb = MixPtr.picLabel;

    // Tickmarks
    qs = MixPtr.Left->slider;
    if (tickmarksOn) {
      qs->setTickmarks(QSlider::Right);
      qs->setTickInterval(10);
    }
    else
      qs->setTickmarks(QSlider::NoMarks);

    QSize VolSBsize = qs->sizeHint();
    qs->setValue(100-MixPtr.volume(0));


    qs->setGeometry( ix, iy+qb->height(), VolSBsize.width(), sliderHeight);
    qs->show();


    // Its a good point to find out the maximum y pos of the slider right here
    if (first) {
      l_i_belowSlider = iy+qb->height() + sliderHeight + 4;
     }

    ix += qs->width();

    // But make sure it isn't linked to the left channel.
    bool l_b_bothSliders =
      (MixPtr.stereo()       == true ) &&
      (MixPtr.stereoLinked() == false);

    QString ToolTipString;
    ToolTipString = MixPtr.name();
    if ( l_b_bothSliders)
      ToolTipString += " (Left)";
    QToolTip::add( qs, ToolTipString );

    // Mark record source(s) and muted channel(s). This is done by using
    // red and green and 'off' LED's
    l_KLed_state = MixPtr.i_KLed_state;
    if (MixPtr.muted()) {
      // Is muted => Off
      l_KLed_state->setState( QceStateLED::Off );
      l_KLed_state->setColor( Qt::black );
    }
    else {
      if (MixPtr.recsrc()) {
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

    if (MixPtr.stereo()  == true) {
      qs = MixPtr.Right->slider;
	
      if (MixPtr.stereoLinked() == false) {
	// Show right slider
	if (tickmarksOn) {
	  qs->setTickmarks(QSlider::Left);
	  qs->setTickInterval(10);
	}
	else {
	  qs->setTickmarks(QSlider::NoMarks);
	}
	QSize VolSBsize = qs->sizeHint();
	qs->setValue(100-MixPtr.volume(1));
	qs->setGeometry( ix, iy+qb->height(), VolSBsize.width(), sliderHeight);

	ix += qs->width();
	ToolTipString = MixPtr.name();
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
  }

  ix += 4;
  iy = qsMaxY +4;
  i_slider_leftRight->setGeometry(0,iy,ix,i_slider_leftRight->sizeHint().height());


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

  iy+=i_slider_leftRight->height();
  i_widget_container->setFixedSize( ix, iy );

  i_widget_container->setUpdatesEnabled(true);

  // tell the Toplevel to do a relayout
  updateRects();

  // And tell anybody who might be interested that the layout has changed (today only the
  // preferences window is interested).
  emit layoutChange();
}


void KMix::slotReadSet1() { slotReadSet(0); }
void KMix::slotReadSet2() { slotReadSet(1); }
void KMix::slotReadSet3() { slotReadSet(2); }
void KMix::slotReadSet4() { slotReadSet(3); }
void KMix::slotWriteSet1() { slotWriteSet(0); }
void KMix::slotWriteSet2() { slotWriteSet(1); }
void KMix::slotWriteSet3() { slotWriteSet(2); }
void KMix::slotWriteSet4() { slotWriteSet(3); }

void KMix::slotReadSet(int num)
{
  i_mixer->Set2HW(num,true);
  i_lbl_setNum->setText( QString(" %1 ").arg(num));
  placeWidgets();
}

void KMix::slotWriteSet(int num)
{
  i_mixer->HW2Set(num);
}

void KMix::createMenu()
{
  QPopupMenu *l_popup_help, *l_popup_file;

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


  i_popup_readSet = new QPopupMenu;
  i_popup_readSet->insertItem(i18n("Profile &1")	, this, SLOT(slotReadSet1()) , Key_1);
  i_popup_readSet->insertItem(i18n("Profile &2")	, this, SLOT(slotReadSet2()) , Key_2);
  i_popup_readSet->insertItem(i18n("Profile &3")	, this, SLOT(slotReadSet3()) , Key_3);
  i_popup_readSet->insertItem(i18n("Profile &4")	, this, SLOT(slotReadSet4()) , Key_4);

  i_popup_writeSet = new QPopupMenu;
  i_popup_writeSet->insertItem(i18n("Profile &1")	, this, SLOT(slotWriteSet1()) , CTRL+Key_1);
  i_popup_writeSet->insertItem(i18n("Profile &2")	, this, SLOT(slotWriteSet2()) , CTRL+Key_2);
  i_popup_writeSet->insertItem(i18n("Profile &3")	, this, SLOT(slotWriteSet3()) , CTRL+Key_3);
  i_popup_writeSet->insertItem(i18n("Profile &4")	, this, SLOT(slotWriteSet4()) , CTRL+Key_4);

  //int QMenuData::insertItem ( const QString & text, QPopupMenu * popup, int id=-1, int index=-1 )
  l_popup_file = new QPopupMenu;
  CHECK_PTR( l_popup_file );
  l_popup_file->insertItem(i18n("&Hide Menubar")    , this, SLOT(hideMenubarCB()) , CTRL+Key_M);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_M),this, SLOT(hideMenubarCB()));

  l_popup_file->insertItem(i18n("&Tickmarks On/Off"), this, SLOT(tickmarksTogCB()), CTRL+Key_T);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_T),this, SLOT(tickmarksTogCB()));

  l_popup_file->insertItem( i18n("&Options...")	, this, SLOT(showOptsCB()) );
  l_popup_file->insertSeparator();
  l_popup_file->insertItem( i18n("Restore Profile")	, i_popup_readSet );
  l_popup_file->insertItem( i18n("Store Profile")	, i_popup_writeSet);
  l_popup_file->insertSeparator();
  l_popup_file->insertItem( i18n("E&xit")          , this, SLOT(quitClickedCB()) , CTRL+Key_Q);
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
    "ALSA port by Nick Lopez (kimo_sabe@usa.net).\n"\
    "HP/UX port by Helge Deller (deller@gmx.de).");
  head += APP_VERSION;

  l_popup_help = helpMenu(i_s_aboutMsg);

  i_menu_main = new KMenuBar( this, "main menu");
  i_menu_main->insertItem( i18n("&File"), l_popup_file );
  i_menu_main->insertSeparator();
  i_menu_main->insertItem( i18n("&Help"), l_popup_help );

  i_popup_balancing = new QPopupMenu;
  i_popup_balancing->insertItem(i18n("&Left")  , this, SLOT(MbalLeftCB()));
  i_popup_balancing->insertItem(i18n("&Center"), this, SLOT(MbalCentCB()));
  i_popup_balancing->insertItem(i18n("&Right") , this, SLOT(MbalRightCB()));
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
	i_point_popup = QCursor::pos();
	qpm->popup(i_point_popup);
	return true;
      }
    }
  }
  return false;
}


/**
   This function returns a suitable context menu to use when the user
   right-clicks on the "free space" of the main window
*/
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
  Mlocal->insertItem( i18n("Restore Profile")	, i_popup_readSet );
  Mlocal->insertItem( i18n("Store Profile")	, i_popup_writeSet);
  Mlocal->insertSeparator();
  Mlocal->insertItem( i18n("&About")           , this, SLOT(launchAboutCB()) );
  Mlocal->insertItem( i18n("&Help")           , this, SLOT(launchHelpCB()) );

  MlocalCreated = true;
  return Mlocal;
}



QPopupMenu* KMix::contextMenu(QObject *o, QObject *e)
{
  if ( o == i_slider_leftRight )
    return i_popup_balancing;

  static bool MlocalCreated=false;
  static QPopupMenu *Mlocal;

  if (o == NULL)
    return NULL;

  if (MlocalCreated) {
    MlocalCreated = false;
    delete Mlocal;
  }

  // Scan mixerChannels for Slider object *o
  QSlider       *qs       = (QSlider*)o;
  MixDevice     *MixFound = 0;
  for ( unsigned int l_i_mixDevice = 0; l_i_mixDevice < i_mixer->size(); l_i_mixDevice++) {
    MixDevice &MixPtr = (*i_mixer)[l_i_mixDevice];
    if ( (MixPtr.Left->slider == qs) || (MixPtr.Right->slider == qs) ) {
      MixFound = &MixPtr;
      break;
    }
  }

  // Have not found slider => return and do not pop up context menu
  if ( MixFound == NULL )
    return ContainerContextMenu(o,e);  // Default context menu


  // else
  Mlocal = new QPopupMenu;
  if (MixFound->muted())
    Mlocal->insertItem(i18n("Un&mute")    , MixFound, SLOT(MvolMuteCB()    ));
  else
    Mlocal->insertItem(i18n("&Mute")      , MixFound, SLOT(MvolMuteCB()    ));
  if (MixFound->stereo()) {
    if (MixFound->stereoLinked())
      Mlocal->insertItem(i18n("&Split")   , MixFound, SLOT(MvolSplitCB()   ));
    else
      Mlocal->insertItem(i18n("Un&split") , MixFound, SLOT(MvolSplitCB()   ));
  }
  if (MixFound->recordable())
    Mlocal->insertItem(i18n("&RecSource") , MixFound, SLOT(MvolRecsrcCB()  ));

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
  i_mixer->setBalance(left,right);
  if (left==100)
    i_slider_leftRight->setValue(right-100);
  else
    i_slider_leftRight->setValue(100-left);
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
  KmConfig->writeEntry( "Balance"    , i_slider_leftRight->value() , true );
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
  i_mixer->sessionSave(sessionConfig);
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
  i_mixer->Set2HW(-1,true);        // Read from hardware
  updateSlidersI();
}

void KMix::updateSlidersI( )
{
  QSlider *qs;
  bool setDisplay = true;
  /* The next line is tricky. Lets explain it:
     Several lines later I will call qs->setValue(). Doing so changes the value of the QSlider.

     This will lead to the emitting of its valueChanged() signal. This again is the hint for
     my code that the user has dragged the slider. As the user expects that the volume changes
     when he drags the slider, this action is indeed being triggered.

     The above scenario shows that everything goes well. Alas - it wasn't the user who dragged
     the slider. We only want to display changes that other programs did - so we are NOT expected
     to write to the hardware again. Using the HW_update(false) does exactly this (avoiding
     writing to the hardware).
  */
  MixChannel::HW_update(false);

  // now update the slider positions...
  for ( unsigned int l_i_mixDevice = 0; l_i_mixDevice < i_mixer->size(); l_i_mixDevice++) {
    MixDevice &MixPtr = (*i_mixer)[l_i_mixDevice];

    qs = MixPtr.Left->slider;
    qs->setValue(100-MixPtr.volume(0));
    if (MixPtr.stereo()  == true) {
      qs = MixPtr.Right->slider;
      qs->setValue(100-MixPtr.volume(1));
    }
    if ( setDisplay) {
      dock_widget->setDisplay(( MixPtr.volume(0) + MixPtr.volume(1) )/2);
      setDisplay = false;
    }
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

  // Quick hack: Always use first channel
  MixDevice     &MixPtr = (*i_mixer)[0];

  if (&MixPtr) {
    // left volume
    l_i_volNew =  MixPtr.volume(0) + val_l_diff;
    if (l_i_volNew > 100) l_i_volNew = 100;
    if (l_i_volNew <   0) l_i_volNew = 0;
    MixPtr.Left->VolChangedI(l_i_volNew);
    if (! MixPtr.stereoLinked() ) {
      // right volume
      l_i_volNew =  MixPtr.volume(1) + val_l_diff;
      if (l_i_volNew > 100) l_i_volNew = 100;
      if (l_i_volNew <   0) l_i_volNew = 0;
      MixPtr.Right->VolChangedI(l_i_volNew);
    }
    updateSliders();
  }

}
