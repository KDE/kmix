/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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

#include "kmixerwidget.h"
#include "mixer.h"
#include "mixdevicewidget.h"
#include "kmixapplet.h"


extern "C"
{
  KPanelApplet* init(QWidget *parent, const QString& configFile)
  {
     KGlobal::locale()->insertCatalogue("kmix");
     return new KMixApplet(configFile, KPanelApplet::Normal,
                           0, parent, "kmixapplet");
  }
}


int KMixApplet::s_instCount = 0;
QTimer *KMixApplet::s_timer = 0;
QList<Mixer> *KMixApplet::s_mixers;

KMixApplet::KMixApplet( const QString& configFile, Type t, int actions,
                        QWidget *parent, const char *name )

   : KPanelApplet( configFile, t, actions, parent, name ),
   m_mixerWidget( 0 ), m_errorLabel( 0 ), m_lockedLayout( 0 )
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
      for ( int dev=0; dev<maxDevices; dev++ )
         for ( int card=0; card<maxCards; card++ )
         {
            Mixer *mixer = Mixer::getMixer( dev, card );
            int mixerError = mixer->grab();
            if ( mixerError!=0 )
               delete mixer;
            else
               s_mixers->append( mixer );
            }
         }

   s_instCount++;

   KGlobal::dirs()->addResourceType( "appicon", KStandardDirs::kde_default("data") + "kmix/pics" );

   // ulgy hack to avoid sending to many updateSize requests to kicker that would freeze it
   m_layoutTimer = new QTimer( this );
   connect( m_layoutTimer, SIGNAL(timeout()), this, SLOT(updateLayoutNow()) );

   // find out to use which mixer
   KConfig *cfg = config();
   cfg->setGroup(0);

   int mixerNum = cfg->readNumEntry( "Mixer", -1 );
   QString mixerName = cfg->readEntry( "MixerName", QString::null );
   Mixer *mixer = 0;
   if ( mixerNum>=0 )
   {
      int m = mixerNum+1;
      for (mixer=s_mixers->first(); mixer!=0; mixer=s_mixers->next())
      {
         if ( mixer->mixerName()==mixerName ) m--;
         if ( m==0 ) break;
      }
   }

   // don't prompt for a mixer if there is just one available
   if ( !mixer && s_mixers->count() == 1 )
       mixer = s_mixers->first();
   
   if ( mixer )
   {
      m_mixerWidget = new KMixerWidget( 0, mixer, mixerName, mixerNum, true, true, this );
      m_mixerWidget->loadConfig( cfg, "Widget" );
      connect( m_mixerWidget, SIGNAL(updateLayout()), this, SLOT(triggerUpdateLayout()));
      connect( s_timer, SIGNAL(timeout()), mixer, SLOT(readSetFromHW()));
   } else
   {
      m_errorLabel = new QPushButton( i18n("Select mixer"), this );
      connect( m_errorLabel, SIGNAL(clicked()), this, SLOT(selectMixer()) );
   }
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
        cfg->writeEntry( "Mixer", s_mixers->find( m_mixerWidget->mixer() ) );
        cfg->writeEntry( "MixerName", m_mixerWidget->mixer()->mixerName() );
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
      int mixerNum = lst.findIndex( res );
      Mixer *mixer = s_mixers->at( mixerNum );
      if (!mixer)
         KMessageBox::sorry( this, i18n("Invalid mixer entered.") );
      else
      {
         delete m_errorLabel;
         m_errorLabel = 0;
         m_mixerWidget = new KMixerWidget( 0, mixer, mixer->mixerName(), mixerNum, true, true, this );
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
      return width;
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

void KMixApplet::preferences()
{
}

#include "kmixapplet.moc"
