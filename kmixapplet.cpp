/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <schimmi@kde.org>
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

#include <stdlib.h>

#include <qlayout.h>
#include <qpixmap.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kaction.h>
#include <qpushbutton.h>
#include <kapp.h>
#include <qwmatrix.h>
#include <kiconloader.h>
#include <qtimer.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <kmessagebox.h>
#include <kcolorbutton.h>
#include <qradiobutton.h>

#include "kmixerwidget.h"
#include "mixer.h"
#include "mixdevicewidget.h"
#include "kmixapplet.h"
#include "colorwidget.h"


extern "C"
{
  KPanelApplet* init(QWidget *parent, const QString& configFile)
  {
     KGlobal::locale()->insertCatalogue("kmix");
     return new KMixApplet(configFile, KPanelApplet::Normal,
                           parent, "kmixapplet");
  }
}


int KMixApplet::s_instCount = 0;
QTimer *KMixApplet::s_timer = 0;
QList<Mixer> *KMixApplet::s_mixers;

#define defHigh "#00FF00"
#define defLow "#FF0000"
#define defBack "#000000"
#define defMutedHigh "#FFFFFF"
#define defMutedLow "#808080"
#define defMutedBack "#000000"

KPanelApplet::Direction reverse(const KPanelApplet::Direction dir) {
   switch (dir) {
   case KPanelApplet::Up   : return KPanelApplet::Down;
   case KPanelApplet::Down : return KPanelApplet::Up;
   case KPanelApplet::Right: return KPanelApplet::Left;
   default                 : return KPanelApplet::Right;
   }
}

KMixApplet::KMixApplet( const QString& configFile, Type t,
                        QWidget *parent, const char *name )

   : KPanelApplet( configFile, t, KPanelApplet::Preferences, parent, name ),
     m_mixerWidget(0), m_errorLabel(0), m_lockedLayout(0), m_pref(0)
{
   // init static vars
   if ( !s_instCount )
   {
      // create mixer list
      s_mixers = new QList<Mixer>;

      // create timers
      s_timer = new QTimer;
      s_timer->start( 500 );

      // get maximum values
      KConfig *config= new KConfig("kcmkmixrc", false);
      config->setGroup("Misc");
      int maxCards = config->readNumEntry( "maxCards", 2 );
      int maxDevices = config->readNumEntry( "maxDevices", 2 );
      delete config;

      // get mixer devices
      s_mixers->setAutoDelete( TRUE );
      QMap<QString,int> mixerNums;
      int drvNum = Mixer::getDriverNum();
      for( int drv=0; drv<drvNum && s_mixers->count()==0; drv++ )
          for ( int dev=0; dev<maxDevices; dev++ )
              for ( int card=0; card<maxCards; card++ )
              {
                  Mixer *mixer = Mixer::getMixer( drv, dev, card );
                  int mixerError = mixer->grab();
                  if ( mixerError!=0 )
                      delete mixer;
                  else {
                      s_mixers->append( mixer );

                      // count mixer nums for every mixer name to identify mixers with equal names
                      mixerNums[mixer->mixerName()]++;
                      mixer->setMixerNum( mixerNums[mixer->mixerName()] );
                  }
              }
   }

   s_instCount++;

   KGlobal::dirs()->addResourceType( "appicon", KStandardDirs::kde_default("data") + "kmix/pics" );

   // ulgy hack to avoid sending to many updateSize requests to kicker that would freeze it
   m_layoutTimer = new QTimer( this );
   connect( m_layoutTimer, SIGNAL(timeout()), this, SLOT(updateLayoutNow()) );

   // get configuration
   KConfig *cfg = config();
   cfg->setGroup(0);

   // get colors
   m_pref = new ColorDialog( this );
   connect( m_pref, SIGNAL(applied()), SLOT(applyColors()) );
   m_customColors = cfg->readBoolEntry( "ColorCustom", false );

   m_colors.high = QColor(cfg->readEntry("ColorHigh", defHigh));
   m_colors.low = QColor(cfg->readEntry( "ColorLow", defLow));
   m_colors.back = QColor(cfg->readEntry( "ColorBack", defBack));

   m_colors.mutedHigh = QColor(cfg->readEntry("MutedColorHigh", defMutedHigh));
   m_colors.mutedLow = QColor(cfg->readEntry( "MutedColorLow", defMutedLow));
   m_colors.mutedBack = QColor(cfg->readEntry( "MutedColorBack", defMutedBack));

   // find out to use which mixer
   mixerNum = cfg->readNumEntry( "Mixer", -1 );
   mixerName = cfg->readEntry( "MixerName", QString::null );
   mixer = 0;
   if ( mixerNum>=0 )
   {
      for (mixer=s_mixers->first(); mixer!=0; mixer=s_mixers->next())
      {
          if ( mixer->mixerName()==mixerName && mixer->mixerNum()==mixerNum ) break;
      }
   }

   // don't prompt for a mixer if there is just one available
   if ( !mixer && s_mixers->count() == 1 )
       mixer = s_mixers->first();

   if ( !mixer )
   {
      m_errorLabel = new QPushButton( i18n("Select mixer"), this );
      connect( m_errorLabel, SIGNAL(clicked()), this, SLOT(selectMixer()) );
   }
   
   //  Find out wether the applet should be reversed
   insideOut = cfg->readBoolEntry("InsideOut", false);
   popupDirectionChange(KPanelApplet::Up);
}

KMixApplet::~KMixApplet()
{
   saveConfig();

   // destroy static vars
   s_instCount--;
   if ( !s_instCount )
   {
      s_mixers->clear();
      delete s_timer;
      delete s_mixers;
   }
}

void KMixApplet::saveConfig()
{
    if ( m_mixerWidget ) {
        KConfig *cfg = config();
        cfg->setGroup( 0 );
        cfg->writeEntry( "Mixer", m_mixerWidget->mixerNum() );
        cfg->writeEntry( "MixerName", m_mixerWidget->mixerName() );

        cfg->writeEntry( "ColorCustom", m_customColors );

        cfg->writeEntry( "ColorHigh", m_colors.high.name() );
        cfg->writeEntry( "ColorLow", m_colors.low.name() );
        cfg->writeEntry( "ColorBack", m_colors.back.name() );

        cfg->writeEntry( "ColorMutedHigh", m_colors.mutedHigh.name() );
        cfg->writeEntry( "ColorMutedLow", m_colors.mutedLow.name() );
        cfg->writeEntry( "ColorMutedBack", m_colors.mutedBack.name() );

        cfg->writeEntry( "InsideOut", insideOut );

        m_mixerWidget->saveConfig( cfg, "Widget" );
        cfg->sync();
    }
}

void KMixApplet::selectMixer()
{
   QStringList lst;

   int n=1;
   for (Mixer *mixer=s_mixers->first(); mixer!=0; mixer=s_mixers->next())
   {
      QString s;
      s.sprintf("%i. %s", n, mixer->mixerName().ascii());
      lst << s;
      n++;
   }

   bool ok = FALSE;
   QString res = QInputDialog::getItem( i18n("Mixers"), i18n( "Available mixers" ), lst,
                                        1, TRUE, &ok, this );
   if ( ok )
   {
      Mixer *mixer = s_mixers->at( lst.findIndex( res ) );
      if (!mixer)
         KMessageBox::sorry( this, i18n("Invalid mixer entered.") );
      else
      {
         delete m_errorLabel;
         m_errorLabel = 0;
         m_mixerWidget = new KMixerWidget( 0, mixer, mixer->mixerName(),
                                           mixer->mixerNum(), true,
                                           popupDirection(), this );
         m_mixerWidget->setColors( m_colors );
         m_mixerWidget->show();
         m_mixerWidget->setGeometry( 0, 0, width(), height() );
         connect( m_mixerWidget, SIGNAL(updateLayout()), this, SLOT(triggerUpdateLayout()));
         connect( s_timer, SIGNAL(timeout()), mixer, SLOT(readSetFromHW()));
         updateLayoutNow();
      }
   }
}

void KMixApplet::triggerUpdateLayout()
{
   if ( m_lockedLayout ) return;
   if ( !m_layoutTimer->isActive() )
     m_layoutTimer->start( 100, TRUE );
}

void KMixApplet::updateLayoutNow()
{
   m_lockedLayout++;
   emit updateLayout();
   saveConfig(); // ugly hack to get the config saved somehow
   m_lockedLayout--;
}

int KMixApplet::widthForHeight( int height) const
{
   int width = 0;
   if ( m_mixerWidget )
   {
      m_mixerWidget->setIcons( height>=32 );
      width = m_mixerWidget->minimumWidth();
   } else
      if ( m_errorLabel )
         width = m_errorLabel->sizeHint().width();

   return width;
}

int KMixApplet::heightForWidth( int width ) const
{
   if ( m_mixerWidget )
   {
      m_mixerWidget->setIcons( width>=32 );
      return m_mixerWidget->minimumHeight();
   } else
      return m_errorLabel->sizeHint().height();
}

void KMixApplet::resizeEvent(QResizeEvent *e)
{
   KPanelApplet::resizeEvent( e );
   if ( m_mixerWidget ) m_mixerWidget->setGeometry( 0, 0, width(), height() );
   if ( m_errorLabel ) m_errorLabel->setGeometry( 0, 0, width(), height() );
}

void KMixApplet::about()
{
}

void KMixApplet::help()
{
}


void KMixApplet::applyColors()
{
    m_colors.high = m_pref->activeHigh->color();
    m_colors.low = m_pref->activeLow->color();
    m_colors.back = m_pref->activeBack->color();

    m_colors.mutedHigh = m_pref->mutedHigh->color();
    m_colors.mutedLow = m_pref->mutedLow->color();
    m_colors.mutedBack = m_pref->mutedBack->color();

    m_customColors = m_pref->customColors->isChecked();

    if ( !m_customColors ) {
        KMixerWidget::Colors cols;
        cols.high = QColor(defHigh);
        cols.low = QColor(defLow);
        cols.back = QColor(defBack);
        cols.mutedHigh = QColor(defMutedHigh);
        cols.mutedLow = QColor(defMutedLow);
        cols.mutedBack = QColor(defMutedBack);

        m_mixerWidget->setColors( cols );
    } else
        m_mixerWidget->setColors( m_colors );
}

void KMixApplet::popupDirectionChange(Direction dir) {
  if (!m_errorLabel) {
    if (m_mixerWidget) delete m_mixerWidget;
    m_mixerWidget = new KMixerWidget( 0, mixer, mixerName, mixerNum, true,
                                      insideOut ? reverse(dir) : dir,
                                      this );
    m_mixerWidget->loadConfig( config(), "Widget" );
    m_mixerWidget->setColors( m_colors );
    connect( m_mixerWidget, SIGNAL(updateLayout()), this, SLOT(triggerUpdateLayout()));
    connect( s_timer, SIGNAL(timeout()), mixer, SLOT(readSetFromHW()));
    m_mixerWidget->show();
  }
}


void KMixApplet::preferences()
{
    if ( !m_pref->isVisible() ) {

        m_pref->activeHigh->setColor( m_colors.high );
        m_pref->activeLow->setColor( m_colors.low );
        m_pref->activeBack->setColor( m_colors.back );

        m_pref->mutedHigh->setColor( m_colors.mutedHigh );
        m_pref->mutedLow->setColor( m_colors.mutedLow );
        m_pref->mutedBack->setColor( m_colors.mutedBack );

        m_pref->defaultLook->setChecked( !m_customColors );
        m_pref->customColors->setChecked( m_customColors );

        m_pref->show();

    } else
        m_pref->raise();
}

#include "kmixapplet.moc"
