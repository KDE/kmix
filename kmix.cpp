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

//  #include <stdio.h>
//  #include <unistd.h>
#include <iostream.h>

#include <kapp.h>
#include <kmenubar.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
//  #include <kwm.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <kstddirs.h>

#include "prefs.h"
#include "kmix-docking.h"
#include "mixdevice.h"
#include "mixer.h"
#include "kmix.h"
#include "version.h"


//#include <qkeycode.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qaccel.h>
#include <qmessagebox.h>
#include <qtooltip.h>


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

  KApplication globalKapp( argc, argv, "kmix" );
  KGlobal::dirs()->addResourceType("icon", KStandardDirs::kde_default("data") + "kmix/pics");

  if (!initonly) {
    // Only initialize GUI, when we only do not just "init"
    // setup dcop communication
    if ( !kapp->dcopClient()->isAttached() )
      kapp->dcopClient()->registerAs("kmix");

    if (globalKapp.isRestored()) {

    // MODE #1 : Restored by Session Management

      int n = 1;
      while (KTMainWindow::canBeRestored(n)) {
        // Read mixer number and set number from session management.
        // This is neccesary, because when the application is restarted
        // by the SM, the
        // should work, too.
        KConfig* scfg = globalKapp.sessionConfig();
        scfg->setGroup("kmixoptions");
        int startSet    = scfg->readNumEntry("startSet",-1);
        int startDevice = scfg->readNumEntry("startDevice",0);
        KMix* kmix = new KMix(startDevice, startSet);
        kmix->restore(n);
        n++;
      }
      return globalKapp.exec();
    }

    else {
      // MODE #3 : Started regulary by the user
      new KMix( mixer_id, SetNumber );
      return globalKapp.exec();
    }
  }
  else {
    // MODE #2 : Only initialize mixer, no GUI
    cout << "Doing initonly ... ";
    Mixer::getMixer( mixer_id, SetNumber );
    cout << "Finished\n";
    return 0;
  }
}

KMix::~KMix()
{
  configSave();

  if (  i_time != 0 ) delete i_time;
}

bool KMix::restore(int number)
{
  if (!canBeRestored(number))
    return false;
  KConfig *config = kapp->sessionConfig();
  if (readPropertiesInternal(config, number)){
    return true;
  }
  return false;

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

  i_time = new QTimer();
  m_defaultPopup = 0L;
  KConfig* config = KApplication::kApplication()->config();

  config->setGroup(0);
  bool mainmenuOn  = config->readNumEntry( "Menubar"  , 1 );
  tickmarksOn = config->readNumEntry( "Tickmarks", 1 );
  int Balance;
  Balance     = config->readNumEntry( "Balance"  , 0 );  // centered by default
  allowDocking= config->readNumEntry( "Docking"  , 0 );
  startDocked = config->readNumEntry( "StartDocked"  , 0 );

  mix = Mixer::getMixer(mixernum, SetNum);
  CHECK_PTR(mix);

  dock_widget = new KMixDockWidget((QString)"kmixdocked", "kmixdocked");
  dock_widget->setMainWindow(this);
  if ( allowDocking ) {
    dock_widget->dock();
  }

  connect ( dock_widget, SIGNAL(quit_clicked()), this, SLOT(quit_myapp()  ));
  connect ( dock_widget, SIGNAL(quickchange(int)), this, SLOT(quickchange_volume(int)  ));
  connect ( kapp , SIGNAL(saveYourself()), this, SLOT(sessionSaveAll() ));

  int mixer_error = mix->grab();
  if ( mixer_error != 0 ) {
    KMessageBox::error(0, mix->errorText(mixer_error), i18n("Mixer failure"));
    mix->errormsg(mixer_error);
    exit(1);
  }

  createWidgets();
  setMenu( createMenu() );
  if (mainmenuOn) menuBar()->show(); else menuBar()->hide();

  if (SetNum > 0) {
    emit newSet( SetNum );
  }

  prefDL     = new Preferences(NULL, this->mix);
  prefDL->menubarChk->setChecked  (mainmenuOn );
  prefDL->tickmarksChk->setChecked(tickmarksOn);
  prefDL->dockingChk->setChecked(allowDocking);

  // Synchronize from KTMW to Prefs and vice versa via signals
  connect(prefDL, SIGNAL(optionsApply()), this  , SLOT(applyOptions()));

  //showOptsCB();  // !!! For faster debugging

  kapp->setMainWidget( this );
  if ( allowDocking && startDocked)
    hide();
  else
    show();
}

// DCOP method
void KMix::activateSet(int l_i_setNum)
{
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
}

void KMix::applyOptions()
{
  if (prefDL->menubarChk->isChecked())
    menuBar()->show();
  else
    menuBar()->hide();

  tickmarksOn = prefDL->tickmarksChk->isChecked();
  allowDocking= prefDL->dockingChk->isChecked();
  //  mix->Set0toHW(); // Do NOT write volume after "applying" Options from config dailog
}

void KMix::createWidgets()
{
  setCaption( mix->mixerName() ); // Window title

  connect( i_time, SIGNAL(timeout()), mix, SLOT(readSetFromHW()) );

  // Create a big container containing every widget of this toplevel
  QWidget* container  = new QWidget(this);
  // and the top layout
  QBoxLayout* toplayout = new QVBoxLayout( container, 3 );

#if 0
  QPixmap l_pixmap_bg;
  l_pixmap_bg = BarIcon("Chicken-Songs-small2");
  container->setBackgroundPixmap( l_pixmap_bg );
#endif

  setView(container);
  container->installEventFilter( this );

  // Create the info line
  QLabel* infoLine = new QLabel(container) ;
  infoLine->setText( mix->mixerName() );
  infoLine->resize( infoLine->sizeHint() );
  //  i_lbl_infoLine->setAlignment(QLabel::AlignRight);
  QToolTip::add( infoLine, i18n("Mixer name") );
  infoLine->installEventFilter( this );


  QLabel* setNum =  new QLabel(container);
  setNum->setText("0"); // set a dummy Text, so that the height() is valid.
  setNum->resize( setNum->sizeHint());
  QToolTip::add( setNum, i18n("Current set number"));
  connect( this, SIGNAL(newSet( int )), setNum, SLOT(setNum( int )) );
  setNum->installEventFilter( this );

  QBoxLayout *infolayout = new QHBoxLayout( toplayout );
  infolayout->addWidget( setNum );
  infolayout->addStretch( 1 );
  infolayout->addWidget( infoLine );

  // Create Sliders (Volume indicators)
  QBoxLayout* mixlayout = new QHBoxLayout( toplayout );
  MixSet mixset = mix->getMixSet();
  MixDevice *mixdevice = mixset.first();
  for ( ; mixdevice != 0; mixdevice = mixset.next())
    {
      MixDeviceWidget *mdw =
        new MixDeviceWidget( mixdevice, container, mixdevice->name() );
      connect( mdw, SIGNAL( newVolume( int, Volume )),
               mix, SLOT( writeVolumeToHW( int, Volume ) ));
      connect( mdw, SIGNAL( newRecsrc(int, bool)),
               mix, SLOT( setRecsrc(int, bool ) ));
      connect( mix, SIGNAL( newRecsrc()),
               mdw, SLOT( updateRecsrc() ));
      connect( i_time, SIGNAL(timeout()), mdw, SLOT(updateSliders()) );
      connect( this, SIGNAL(updateTicks(bool)), mdw, SLOT(updateTicks(bool)) );
      if( mixdevice->num() == mix->masterDevice() )
        connect( mix, SIGNAL(newBalance(Volume)), mdw, SLOT(setVolume(Volume)) );
      mdw->installEventFilter( this );
      mixlayout->addWidget( mdw );
    }

  // Create the Left-Right-Slider, add Tooltip and Context menu
  LeftRightSB = new QSlider( -100, 100, 25, 0,\
			     QSlider::Horizontal, container, "RightLeft");
  connect( LeftRightSB, SIGNAL(valueChanged(int)), \
	   this, SLOT(MbalChangeCB(int)));
  QToolTip::add( LeftRightSB, i18n("Left/Right balancing") );
  LeftRightSB->installEventFilter(this);

  toplayout->addWidget( LeftRightSB );
  toplayout->activate();

  i_time->start( 1000 );
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
//    mix->Set2Set0(num,true);
//    mix->Set0toHW();
  emit newSet( num );
}

void KMix::slotWriteSet(int /*num*/)
{
//    mix->Set0toSet(num);
}

KMenuBar* KMix::createMenu()
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


  i_m_readSet = new QPopupMenu;
  i_m_readSet->insertItem(i18n("Profile &1")	, this, SLOT(slotReadSet1()) , Key_1);
  i_m_readSet->insertItem(i18n("Profile &2")	, this, SLOT(slotReadSet2()) , Key_2);
  i_m_readSet->insertItem(i18n("Profile &3")	, this, SLOT(slotReadSet3()) , Key_3);
  i_m_readSet->insertItem(i18n("Profile &4")	, this, SLOT(slotReadSet4()) , Key_4);

  i_m_writeSet = new QPopupMenu;
  i_m_writeSet->insertItem(i18n("Profile &1")	, this, SLOT(slotWriteSet1()) , CTRL+Key_1);
  i_m_writeSet->insertItem(i18n("Profile &2")	, this, SLOT(slotWriteSet1()) , CTRL+Key_2);
  i_m_writeSet->insertItem(i18n("Profile &3")	, this, SLOT(slotWriteSet1()) , CTRL+Key_3);
  i_m_writeSet->insertItem(i18n("Profile &4")	, this, SLOT(slotWriteSet1()) , CTRL+Key_4);

  //int QMenuData::insertItem ( const QString & text, QPopupMenu * popup, int id=-1, int index=-1 )
  QPopupMenu* Mfile = new QPopupMenu;
  CHECK_PTR( Mfile );
  Mfile->insertItem(i18n("&Hide Menubar")    , this, SLOT(toggleMenubarCB()) , CTRL+Key_M);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_M),this, SLOT(toggleMenubarCB()));

  Mfile->insertItem(i18n("&Tickmarks On/Off"), this, SLOT(tickmarksTogCB()), CTRL+Key_T);
  qAcc->connectItem( qAcc->insertItem(CTRL+Key_T),this, SLOT(tickmarksTogCB()));

  Mfile->insertItem( i18n("&Options...")	, this, SLOT(showOptsCB()) );
  Mfile->insertSeparator();
  Mfile->insertItem( i18n("Restore Profile")	, i_m_readSet );
  Mfile->insertItem( i18n("Store Profile")	, i_m_writeSet);
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
    "ALSA port by Nick Lopez (kimo_sabe@usa.net).\n"\
    "HP/UX port by Helge Deller (deller@gmx.de).");
  head += APP_VERSION;

#if QT_VERSION >= 200
  QPopupMenu* Mhelp = helpMenu(i_s_aboutMsg);
#else
  QPopupMenu* Mhelp = globalKapp.helpMenu(true, i_s_aboutMsg);
#endif

  CHECK_PTR( Mhelp );

  KMenuBar *mainmenu = new KMenuBar( this, "main menu");
  CHECK_PTR( mainmenu );
  mainmenu->insertItem( i18n("&File"), Mfile );
  mainmenu->insertSeparator();
  mainmenu->insertItem( i18n("&Help"), Mhelp );

  Mbalancing = new QPopupMenu;
  CHECK_PTR( Mbalancing );
  Mbalancing->insertItem(i18n("&Left")  , this, SLOT(MbalLeftCB()));
  Mbalancing->insertItem(i18n("&Center"), this, SLOT(MbalCentCB()));
  Mbalancing->insertItem(i18n("&Right") , this, SLOT(MbalRightCB()));

  return mainmenu;
}

void KMix::tickmarksTogCB()
{
  tickmarksOn=!tickmarksOn;
  emit updateTicks( tickmarksOn );
}

void KMix::toggleMenubarCB()
{
  if (menuBar()->isVisible())
    menuBar()->hide();
  else
    menuBar()->show();
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
  kapp->invokeHTMLHelp("", "");
}

void KMix::launchAboutCB()
{
  QMessageBox::about( 0L, kapp->caption(), i_s_aboutMsg );
}


bool KMix::eventFilter(QObject *o, QEvent *e)
{
  // Lets see, if we have a "Right mouse button press"
  if (e->type() == QEvent::MouseButtonPress)
    {
      QMouseEvent *qme = (QMouseEvent*)e;
      if (qme->button() == RightButton)
        {
          QPopupMenu *qpm = contextMenu(o);

          if (qpm)
            {
              QPoint KCMpopup_point = QCursor::pos();
              qpm->popup(KCMpopup_point);
              return true;
            }
        }
    }
  else if (e->type() == QEvent::Resize)
    {
      int newwid = view()->layout()->sizeHint().width();
      if( width() < newwid )
        resize( newwid, height() );
    }

  return false;
}


QPopupMenu* KMix::ContainerContextMenu()
{
  if ( m_defaultPopup ) {
    delete m_defaultPopup;
  }
  m_defaultPopup = new QPopupMenu;
  if ( menuBar()->isVisible() )
    m_defaultPopup->insertItem( i18n("&Hide Menubar") , this, SLOT(toggleMenubarCB()) );
  else
    m_defaultPopup->insertItem( i18n("&Show Menubar") , this, SLOT(toggleMenubarCB()) );
  m_defaultPopup->insertItem( i18n("&Options...")        , this, SLOT(showOptsCB()) );
  m_defaultPopup->insertSeparator();
  m_defaultPopup->insertItem( i18n("Restore Profile")	, i_m_readSet );
  m_defaultPopup->insertItem( i18n("Store Profile")	, i_m_writeSet);
  m_defaultPopup->insertSeparator();
  m_defaultPopup->insertItem( i18n("&About")           , this, SLOT(launchAboutCB()) );
  m_defaultPopup->insertItem( i18n("&Help")           , this, SLOT(launchHelpCB()) );

  return m_defaultPopup;
}



QPopupMenu* KMix::contextMenu(QObject *o)
{
  if ( o == LeftRightSB )
    return Mbalancing;

//    if ( o->isA( "MixDeviceWidget" ) )
//      return ((MixDeviceWidget*)o)->popupMenu();

  return ContainerContextMenu();  // Default context menu
}

void KMix::MbalCentCB()
{
  setBalance( 0 );
}

void KMix::MbalLeftCB()
{
  setBalance( -100 );
}

void KMix::MbalRightCB()
{
  setBalance( 100 );
}

void KMix::MbalChangeCB(int pos)
{
  setBalance( pos );
}



void KMix::setBalance(int balance)
{
  mix->setBalance( balance );
  LeftRightSB->setValue( balance );
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
  KConfig* config = kapp->config();
  config->setGroup(0);
  config->writeEntry( "Balance"    , LeftRightSB->value() , true );
  config->writeEntry( "Menubar"    , menuBar()->isVisible() );
  config->writeEntry( "Tickmarks"  , tickmarksOn );
  config->writeEntry( "Docking"    , allowDocking);
  bool iv = isVisible();
  config->writeEntry( "StartDocked", !iv);

  if (sessionConfig) {
    // Save session specific data only when needed
    KConfig* scfg = kapp->sessionConfig();
    scfg->setGroup("kmixoptions");
    scfg->writeEntry("startSet", startSet);
    scfg->writeEntry("startDevice", startDevice);
  }
  mix->sessionSave(sessionConfig);
  config->sync();
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
    kapp->setTopWidget( this );
    return ;
  }
}


void KMix::updateSliders( )
{

}



void KMix::quit_myapp()
{
  configSave();
  kapp->quit();
}

void KMix::quickchange_volume(int diff)
{
  MixDevice *md = (*mix)[1];

  md->setVolume( Volume::LEFT, md->leftVolume() + diff );
  md->setVolume( Volume::RIGHT, md->rightVolume() + diff );
}

#include "kmix.moc"
