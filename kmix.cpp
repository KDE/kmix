static char rcsid[]="$Id$";

// Thanks for taking a look in here. :-)

/*
 * Copyright by Christian Esken 1996-97
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdio.h>
#include <iostream.h>
#include <kiconloader.h>
#include <klocale.h>

#include "mixer.h"
#include "kmix.h"
#include "kmix.moc"
#include "version.h"


#include <qkeycode.h>
#include <qlabel.h>
#include <qaccel.h>


KApplication *globalKapp;
KIconLoader  *globalKIL;

bool		ReadFromSet=false;		// !!! Sets not implemented yet
char		SetNumber;
extern char	KMixErrors[6][100];

int main(int argc, char **argv)
{
  globalKapp = new KApplication( argc, argv, "kmix" );
  globalKIL  = globalKapp->getIconLoader();
  KMix *kmix;


  /* Parse the command line arguments */
  for (int i=0 ; i<argc; i++) {
    if (strcmp(argv[i],"-V") == 0) {
      cout << "kmix " << rcsid << '\n';
      exit(0);
    }
    else if (strcmp(argv[i],"-r") == 0) {
      ReadFromSet = true;
      SetNumber   = 0;
    }
    else if (strcmp(argv[i],"-R") == 0 && i+1<argc) {
      /* -R is the command to read in a specified set.
       * The set number is given as the next argument.
       */
      i=i+1;
      ReadFromSet = true;
      SetNumber   = atoi(argv[i]);
    }
  }

  if (kapp->isRestored()){
      int n = 1;
      while (KTopLevelWidget::canBeRestored(n)){
        kmix = new KMix(DEFAULT_MIXER);
        kmix->restore(n);
        n++;
      }
   }
  else {
    if (argc > 1)
      kmix = new KMix(argv[argc - 1]);
    else
      kmix = new KMix(DEFAULT_MIXER);
  }

  return globalKapp->exec();
}

KMix::~KMix()
{
  sessionSave();
  delete mainmenu;
}

KMix::KMix(char *mixername)
{
  KmConfig=KApplication::getKApplication()->getConfig();
  mainmenuOn  = true;  // obsolete?
  tickmarksOn = true;  // have to check KConfig specification to make sure

  mainmenuOn  = KmConfig->readNumEntry( "Menubar"  , 1 );
  tickmarksOn = KmConfig->readNumEntry( "Tickmarks", 1 );
  int Balance;
  Balance     = KmConfig->readNumEntry( "Balance"  , 0 );  // centered by default

  KCM = new KCmManager(this);
  CHECK_PTR(KCM);
  mix = new Mixer(mixername);
  CHECK_PTR(mix);

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
  connect(prefDL, SIGNAL(optionsApply()), this, SLOT(applyOptions()));

  globalKapp->setMainWidget( this );
  show();
}

void KMix::applyOptions()
{
  mainmenuOn  = prefDL->menubarChk->isChecked();
  tickmarksOn = prefDL->tickmarksChk->isChecked();

  KmConfig->writeEntry( "Menubar"    , mainmenuOn  , true );
  KmConfig->writeEntry( "Tickmarks"  , tickmarksOn , true );
  KmConfig->sync();

  placeWidgets();
}

void KMix::createWidgets()
{
  QPixmap miniDevPM;
  QPixmap WMminiIcon = globalKIL->loadIcon("mixer_mini.xpm");
//   KWM::setMiniIcon(this->winId(), WMminiIcon);

  // keep this enum local. It is really only needed here
  enum {audioIcon, bassIcon, cdIcon, extIcon, microphoneIcon,
	midiIcon, recmonIcon,trebleIcon, unknownIcon, volumeIcon };

  char DefaultMixerIcons[]={
    volumeIcon,		bassIcon,	trebleIcon,	midiIcon,	audioIcon,
    unknownIcon,	extIcon,	microphoneIcon,	cdIcon,		recmonIcon,
    audioIcon,		recmonIcon,	recmonIcon,	recmonIcon,	extIcon,
    extIcon,		extIcon
  };
  const unsigned char numDefaultMixerIcons=17;

  // Init DnD: Set up drop zone and drop handler
  dropZone = new KDNDDropZone( this, DndURL );
  connect( dropZone, SIGNAL( dropAction( KDNDDropZone* )), 
	   SLOT( onDrop( KDNDDropZone*)));

  // Window title
  setCaption( "KMix" );

  // Create a big container containing every widget of this toplevel
  Container  = new QWidget(this);
  setView(Container);

  // Create Menu
  createMenu();
  setMenu(mainmenu);
  // Create Sliders (Volume indicators)

  MixDevice *MixPtr = mix->First;
  while (MixPtr)
    {
      // If you encounter a relayout signal from a mixer device, obey blindly ;-)
      connect((QObject*)MixPtr, SIGNAL(relayout()), this, SLOT(placeWidgets()));

      int devnum = MixPtr->device_num;

      // Figure out default icon
      unsigned char iconnum;
      if (devnum < numDefaultMixerIcons)
	iconnum=DefaultMixerIcons[devnum];
      else
	iconnum=unknownIcon;
      switch (iconnum)
	{ // TODO: Should be replaceable by user.
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

      QSlider *VolSB = new QSlider( 0, 100, 10, MixPtr->Left->volume,\
				    QSlider::Vertical, Container, "VolL");

      MixPtr->Left->slider = VolSB;  // Remember the Slider (for the eventFilter)
      connect( VolSB, 	SIGNAL(valueChanged(int)), MixPtr->Left, SLOT(VolChanged(int)));

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

    if (MixPtr->is_stereo  == true)
      {
	qs = MixPtr->Right->slider;
	  
	if (MixPtr->StereoLink == false)
	  { // Show right slider
	    if (tickmarksOn)
	      {
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
    qb->move((int)((ix+old_x-qb->width())/2),iy);

    first=false;
    MixPtr=MixPtr->Next;
  }

  ix += 4; // !!! Hack. TODO chris
  iy = qsMaxY;
  LeftRightSB->setGeometry(0,iy,ix,LeftRightSB->sizeHint().height());

  iy+=LeftRightSB->height();
  Container->setFixedSize( ix, iy );
  updateRects();
}



void KMix::createMenu()
{

  QAccel *qAcc = new QAccel( this );

  Mfile = new QPopupMenu;
  CHECK_PTR( Mfile );
  Mfile->insertItem(klocale->translate("Hide Menubar")    , this, SLOT(hideMenubarCB()) , CTRL+Key_M);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_M),this, SLOT(hideMenubarCB()));

  Mfile->insertItem(klocale->translate("Tickmarks On/Off"), this, SLOT(tickmarksTogCB()), CTRL+Key_T);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_T),this, SLOT(tickmarksTogCB()));

  Mfile->insertItem( klocale->translate("&Options")       , this, SLOT(showOptsCB()) );
  Mfile->insertSeparator();
  Mfile->insertItem( klocale->translate("&Quit")          , this, SLOT(quitClickedCB()) , CTRL+Key_Q);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_Q),this, SLOT(quitClickedCB()));
  Mhelp = new QPopupMenu;
  CHECK_PTR( Mhelp );
  Mhelp->insertItem( klocale->translate("&Contents"), this, SLOT(launchHelpCB()), Key_F1);
  qAcc->connectItem( qAcc->insertItem(Key_F1),this, SLOT(launchHelpCB()));
  Mhelp->insertSeparator();
  Mhelp->insertItem( klocale->translate("&About"), this, SLOT(aboutClickedCB()));
  Mhelp->insertItem( klocale->translate("&About Qt..."), this, SLOT(aboutqt()));

  mainmenu = new KMenuBar( this, "main menu");
  CHECK_PTR( mainmenu );
  mainmenu->insertItem( klocale->translate("&File"), Mfile );
  mainmenu->insertSeparator();
  mainmenu->insertItem( klocale->translate("&Help"), Mhelp );

  Mbalancing = new QPopupMenu;
  CHECK_PTR( Mbalancing );
  Mbalancing->insertItem(klocale->translate("&Left")  , this, SLOT(MbalLeftCB()));
  Mbalancing->insertItem(klocale->translate("&Center"), this, SLOT(MbalCentCB()));
  Mbalancing->insertItem(klocale->translate("&Right") , this, SLOT(MbalRightCB()));
}

void KMix::tickmarksTogCB()
{
  tickmarksOn=!tickmarksOn;
  placeWidgets();
}

void KMix::hideMenubarCB()
{
  mainmenuOn=!mainmenuOn;
  prefDL->updateChannelConfWindow();
  placeWidgets();
}

void KMix::showOptsCB()
{
  prefDL->show();
}

void KMix::quitClickedCB()
{
  //  int  ok = KMsgBox::yesNo(NULL, "Confirm", "Quit KMix?" );
  //  if (ok==1) 
  delete this;
  exit(0);
}

void KMix::aboutClickedCB()
{
  QString msg,head;
  char vers[50];
  sprintf (vers,"%.2f", APP_VERSION);
  
  msg  = "kmix ";
  msg += vers;
  msg += "\n(C) 1997 by Christian Esken (esken@kde.org).\n\n" \
    "Sound mixer panel for the KDE Desktop Environment.\n"\
    "This program is in the GPL.\n"
    "SGI Port done by Paul Kendall (paul@orion.co.nz)";

  head = "About kmix ";
  head += vers;

  QMessageBox::about(this, head, msg );
}

void KMix::aboutqt()
{
  QMessageBox::aboutQt(this);
}

void KMix::launchHelpCB()
{
  globalKapp->invokeHTMLHelp("kmedia/kmix.html", "");
}




bool KMix::eventFilter(QObject *o, QEvent *e)
{
  // Lets see, if we have a "Right mouse button press"
  if (e->type() == Event_MouseButtonPress)
    {
      QMouseEvent *qme = (QMouseEvent*)e;
      if (qme->button() == RightButton)
	{
	  QPopupMenu *qpm = contextMenu(o);

	  if (qpm) {
	    // QPoint p1 =  qme->pos();
	    // cerr << "Right Mouse button pressed at (" << p1.x() << "," << p1.y() << ").\n";
	    KCMpopup_point = QCursor::pos();
	    qpm->popup(KCMpopup_point);
	    return true;
	  }
	}
    }
  return false;
}


QPopupMenu* KMix::contextMenu(QObject *o)
{
  if ( o == LeftRightSB )
    {
      return Mbalancing;
    }

  static bool MlocalCreated=false;
  static QPopupMenu *Mlocal;

  if (o == NULL) {
//    cerr << "ContextMenu for NULL object requested!"; 
    return NULL;
  }
  else {
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
    if ( MixFound == NULL)
      return NULL;

    // else
    Mlocal = new QPopupMenu;
    CHECK_PTR( Mlocal );
    if (MixFound->is_muted)
      Mlocal->insertItem(klocale->translate("Un&mute")    , MixPtr, SLOT(MvolMuteCB()    ));
    else
      Mlocal->insertItem(klocale->translate("&Mute")      , MixPtr, SLOT(MvolMuteCB()    ));
    if (MixFound->is_stereo)
      if (MixFound->StereoLink)
	Mlocal->insertItem(klocale->translate("&Split")   , MixPtr, SLOT(MvolSplitCB()   ));
      else
	Mlocal->insertItem(klocale->translate("Un&split") , MixPtr, SLOT(MvolSplitCB()   ));
    if (MixFound->is_recordable)
      Mlocal->insertItem(klocale->translate("&RecSource") , MixPtr, SLOT(MvolRecsrcCB()  ));

    MlocalCreated = true;
    return Mlocal;
  }
}





void KMix::onDrop( KDNDDropZone* _zone )
{
  QStrList strlist;
  KURL *url;

//  cerr << "URLs dropped on KMix!\n";
  strlist = _zone->getURLList();

  url = new KURL( strlist.first() );
//  cout << url->path() << "\n";
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


// first aspects of session management
void KMix::sessionSave()
{
  KmConfig->writeEntry( "Balance"  , LeftRightSB->value() , true );
  KmConfig->sync();

}
