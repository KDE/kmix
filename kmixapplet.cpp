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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdlib.h>

#include <qgroupbox.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qcheckbox.h>

#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <qpushbutton.h>
#include <kapplication.h>
#include <qwmatrix.h>
#include <kiconloader.h>
#include <qtimer.h>
#include <kinputdialog.h>
#include <qlabel.h>
#include <kmessagebox.h>
#include <kcolorbutton.h>
#include <qradiobutton.h>
#include <kglobalsettings.h>
#include <kbugreport.h>
#include <kaboutdata.h>
#include <kaboutapplication.h>

#include "kmixerwidget.h"
#include "mixer.h"
#include "mixdevicewidget.h"
#include "kmixapplet.h"
#include "colorwidget.h"
#include "version.h"


extern "C"
{
  KPanelApplet* init(QWidget *parent, const QString& configFile)
  {
     KGlobal::locale()->insertCatalogue("kmix");
     return new KMixApplet(configFile, KPanelApplet::Normal,
                           parent, "kmixapplet");
  }
}

KMixApplet *kmixApp = 0L;

int KMixApplet::s_instCount = 0;
QTimer *KMixApplet::s_timer = 0;
QPtrList<Mixer> *KMixApplet::s_mixers;

static const QColor highColor = KGlobalSettings::baseColor();
static const QColor lowColor = KGlobalSettings::highlightColor();
static const QColor backColor = "#000000";
static const QColor mutedHighColor = "#FFFFFF";
static const QColor mutedLowColor = "#808080";
static const QColor mutedBackColor = "#000000";

AppletConfigDialog::AppletConfigDialog( QWidget * parent, const char * name )
   : KDialogBase( KDialogBase::Plain, QString::null,
                  KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                  KDialogBase::Ok, parent, name, false, true)
{
   setPlainCaption(i18n("Configure - Mixer Applet"));
   QFrame* page = plainPage();
   QVBoxLayout *topLayout = new QVBoxLayout(page);
   colorWidget = new ColorWidget(page);
   topLayout->addWidget(colorWidget);
   setUseCustomColors(false);
}

void AppletConfigDialog::slotOk()
{
    slotApply();
    KDialogBase::slotOk();
}

void AppletConfigDialog::slotApply()
{
    emit applied();
}

void AppletConfigDialog::setActiveColors(const QColor& high, const QColor& low, const QColor& back)
{
    colorWidget->activeHigh->setColor(high);
    colorWidget->activeLow->setColor(low);
    colorWidget->activeBack->setColor(back);
}

void AppletConfigDialog::activeColors(QColor& high, QColor& low, QColor& back) const
{
    high = colorWidget->activeHigh->color();
    low  = colorWidget->activeLow->color();
    back = colorWidget->activeBack->color();
}

void AppletConfigDialog::setMutedColors(const QColor& high, const QColor& low, const QColor& back)
{
    colorWidget->mutedHigh->setColor(high);
    colorWidget->mutedLow->setColor(low);
    colorWidget->mutedBack->setColor(back);
}

void AppletConfigDialog::mutedColors(QColor& high, QColor& low, QColor& back) const
{
    high = colorWidget->mutedHigh->color();
    low  = colorWidget->mutedLow->color();
    back = colorWidget->mutedBack->color();
}

void AppletConfigDialog::setUseCustomColors(bool custom)
{
    colorWidget->customColors->setChecked(custom);
    colorWidget->activeColors->setEnabled(custom);
    colorWidget->mutedColors->setEnabled(custom);
}

bool AppletConfigDialog::useCustomColors() const
{
    return colorWidget->customColors->isChecked();
}

void AppletConfigDialog::setReverseDirection(bool reverse)
{
    colorWidget->reverseDirection->setChecked(reverse);
}

bool AppletConfigDialog::reverseDirection() const
{
    return colorWidget->reverseDirection->isChecked();
}

KPanelApplet::Direction KMixApplet::checkReverse(Direction dir) {
   if( reversedDir ) {
       switch (dir) {
       case KPanelApplet::Up   : return KPanelApplet::Down;
       case KPanelApplet::Down : return KPanelApplet::Down;
       case KPanelApplet::Right: return KPanelApplet::Left;
       default                 : return KPanelApplet::Left;
       }
   } else {
       switch (dir) {
       case KPanelApplet::Up   : return KPanelApplet::Up;
       case KPanelApplet::Down : return KPanelApplet::Up;
       case KPanelApplet::Right: return KPanelApplet::Right;
       default                 : return KPanelApplet::Right;
       }
   }
}

KMixApplet::KMixApplet( const QString& configFile, Type t,
                        QWidget *parent, const char *name )

   : KPanelApplet( configFile, t, KPanelApplet::Preferences | KPanelApplet::ReportBug | KPanelApplet::About, parent, name ),
     m_mixerWidget(0), m_errorLabel(0), m_lockedLayout(0), m_pref(0),
     m_aboutData( "kmix", I18N_NOOP("KMix Panel Applet"),
                         APP_VERSION, "Mini Sound Mixer Applet", KAboutData::License_GPL,
                         I18N_NOOP( "(c) 1996-2000 Christian Esken\n(c) 2000-2003 Christian Esken, Stefan Schimanski") )
{
	kmixApp = this;
	
	// init static vars
	if ( !s_instCount )
	{
		// create mixer list
		s_mixers = new QPtrList<Mixer>;
		
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
		{
			for ( int dev=0; dev<maxDevices; dev++ )
			{
				for ( int card=0; card<maxCards; card++ )
				{
					Mixer *mixer = Mixer::getMixer( drv, dev, card );
					int mixerError = mixer->grab();
					if ( mixerError!=0 )
					{
						delete mixer;
						continue;
					}
					s_mixers->append( mixer );
					
					// count mixer nums for every mixer name to identify mixers with equal names
					mixerNums[mixer->mixerName()]++;
					mixer->setMixerNum( mixerNums[mixer->mixerName()] );
				}
			}
		}
	}
	
	s_instCount++;
	
	KGlobal::dirs()->addResourceType( "appicon", KStandardDirs::kde_default("data") + "kmix/pics" );
	
	// ulgy hack to avoid sending to many updateSize requests to kicker that would freeze it
	m_layoutTimer = new QTimer( this );
	connect( m_layoutTimer, SIGNAL(timeout()), this, SLOT(updateLayoutNow()) );
	
	// get configuration
   KConfig *cfg = kmixApp->config();
	cfg->setGroup(0);
	
	// get colors
	m_customColors = cfg->readBoolEntry( "ColorCustom", false );
	
	m_colors.high = cfg->readColorEntry("ColorHigh", &highColor);
	m_colors.low = cfg->readColorEntry("ColorLow", &lowColor);
	m_colors.back = cfg->readColorEntry("ColorBack", &backColor);
	
	m_colors.mutedHigh = cfg->readColorEntry("MutedColorHigh", &mutedHighColor);
	m_colors.mutedLow = cfg->readColorEntry("MutedColorLow", &mutedLowColor);
	m_colors.mutedBack = cfg->readColorEntry("MutedColorBack", &mutedBackColor);
	
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
		m_errorLabel = new QPushButton( i18n("Select Mixer"), this );
		connect( m_errorLabel, SIGNAL(clicked()), this, SLOT(selectMixer()) );
	}
	
	//  Find out wether the applet should be reversed
	reversedDir = cfg->readBoolEntry("ReversedDirection", false);
	
	popupDirectionChange(KPanelApplet::Up);
	
	m_aboutData.addCredit( I18N_NOOP( "For detailed credits, please refer to the About information of the KMix program" ) );
}

KMixApplet::~KMixApplet()
{
   saveConfig();

   // destroy static vars
   s_instCount--;
   if ( !s_instCount )
   {
      QPtrListIterator<Mixer> it( *s_mixers );
      for ( ; it.current(); ++it )
         it.current()->release();

      s_mixers->clear();
      delete s_timer;
      delete s_mixers;
   }
}

void KMixApplet::saveConfig()
{
    if ( m_mixerWidget ) {
        KConfig *cfg = kmixApp->config();
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

        cfg->writeEntry( "ReversedDirection", reversedDir );

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
   QString res = KInputDialog::getItem( i18n("Mixers"),
                                        i18n("Available mixers:"),
					lst, 1, FALSE, &ok, this );
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
                                           popupDirection(), MixDevice::SLIDER, this );
         setColors();
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
    KAboutApplication aboutDlg(&m_aboutData);
    aboutDlg.exec();
}

void KMixApplet::help()
{
}

void KMixApplet::setColors()
{
    if ( !m_customColors ) {
        KMixerWidget::Colors cols;
        cols.high = highColor;
        cols.low = lowColor;
        cols.back = backColor;
        cols.mutedHigh = mutedHighColor;
        cols.mutedLow = mutedLowColor;
        cols.mutedBack = mutedBackColor;

        m_mixerWidget->setColors( cols );
    } else
        m_mixerWidget->setColors( m_colors );
}

void KMixApplet::popupDirectionChange(Direction dir) {
  if (!m_errorLabel) {
    if (m_mixerWidget) delete m_mixerWidget;
    m_mixerWidget = new KMixerWidget( 0, mixer, mixerName, mixerNum, true,
                                      checkReverse(dir), MixDevice::ALL, this );
    m_mixerWidget->loadConfig( config(), "Widget" );
    setColors();
    connect( m_mixerWidget, SIGNAL(updateLayout()), this, SLOT(triggerUpdateLayout()));
    connect( s_timer, SIGNAL(timeout()), mixer, SLOT(readSetFromHW()));
    m_mixerWidget->show();
  }
}


void KMixApplet::preferences()
{
    if ( !m_pref )
    {
        m_pref = new AppletConfigDialog( this );
        connect(m_pref, SIGNAL(finished()), SLOT(preferencesDone()));
        connect( m_pref, SIGNAL(applied()), SLOT(applyPreferences()) );

        m_pref->setActiveColors(m_colors.high, m_colors.low, m_colors.back);
        m_pref->setMutedColors(m_colors.mutedHigh, m_colors.mutedLow, m_colors.mutedBack);

        m_pref->setUseCustomColors( m_customColors );
        m_pref->setReverseDirection( reversedDir );

    }

    m_pref->show();
    m_pref->raise();
}

void KMixApplet::reportBug()
{
    KBugReport bugReportDlg(this, true, &m_aboutData);
    bugReportDlg.exec();
}

void KMixApplet::preferencesDone()
{
    m_pref->delayedDestruct();
    m_pref = 0;
}

void KMixApplet::applyPreferences()
{
    if (!m_pref)
        return;

    m_pref->activeColors(m_colors.high, m_colors.low, m_colors.back);
    m_pref->mutedColors(m_colors.mutedHigh, m_colors.mutedLow, m_colors.mutedBack);
    m_customColors = m_pref->useCustomColors();

    reversedDir = m_pref->reverseDirection();
    QSize si = m_mixerWidget->size();
    popupDirectionChange( popupDirection());
    if( popupDirection() == Up || popupDirection() == Down )
        m_mixerWidget->setIcons( si.height()>=32 );
    else
        m_mixerWidget->setIcons( si.width()>=32 );

    m_mixerWidget->resize( si );
    setColors();
}

#include "kmixapplet.moc"

// vim: sw=3 ts=3
