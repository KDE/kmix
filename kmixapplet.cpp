/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <schimmi@kde.org>
 * Copyright (C) 2004 Christian Esken <esken@kde.org>
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

// System
#include <stdlib.h>

// QT
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qwmatrix.h>


// KDE
#include <kaboutapplication.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kapplication.h>
#include <kbugreport.h>
#include <kcolorbutton.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kglobalaccel.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

// KMix
#include "colorwidget.h"
#include "kmixapplet.h"
#include "kmixtoolbox.h"
#include "mdwslider.h"
#include "mixdevicewidget.h"
#include "mixer.h"
#include "version.h"
#include "viewapplet.h"


extern "C"
{
  KPanelApplet* init(QWidget *parent, const QString& configFile)
  {
     KGlobal::locale()->insertCatalogue("kmix");
     return new KMixApplet(configFile, KPanelApplet::Stretch,
                           parent, "kmixapplet");
  }
}

KMixApplet *kmixApp = 0L;

int KMixApplet::s_instCount = 0;
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
    //kdDebug(67100) << "KMixApplet::KMixApplet()" << endl;
    kmixApp = this;

    // init static vars
    if ( !s_instCount ) {
	int driverWithMixer = -1;
	bool multipleDriversActive = false;

	QString driverInfo = "";
	QString driverInfoUsed = "";

	QString m_hwInfoString;
	// create mixer list
	s_mixers = new QPtrList<Mixer>;
		
		
	// get mixer devices
	s_mixers->setAutoDelete( TRUE );
	QMap<QString,int> mixerNums;

	int drvNum = Mixer::getDriverNum();

	// following line and loop identical with kmix.cpp
	bool autodetectionFinished = false;
	for( int drv=0; drv<drvNum; drv++ )
	{
	    if ( autodetectionFinished ) {
		// sane exit from loop
		break;
	    }
	    bool drvInfoAppended = false;
	    // The "64" below is just a "silly" number:
	    // The loop will break as soon as an error is detected (e.g. on 3rd loop when 2 soundcards are installed)
	    for( int dev=0; dev<64; dev++ )
	    {
		//kdDebug(67100) << "KMixApplet::KMixApplet() detecting drv=" << drv << "dev=" << dev << endl;
		Mixer *mixer = Mixer::getMixer( drv, dev, 0 );
		int mixerError = mixer->grab();
		if ( mixerError!=0 )
		{
		    if ( s_mixers->count() > 0 ) {
			// why not always ?!? !!
			delete mixer;
			mixer = 0;
		    }

		    /* If we get here, we *assume* that we probed the last dev of the current soundcard driver.
		     * We cannot be sure 100%, probably it would help to check the "mixerError" variable. But I
		     * currently don't see an error code that needs to be handled explicitely.
		     *
		     * Lets decide if we the autoprobing shall continue:
		     */
		    if ( s_mixers->count() == 0 ) {
			// Simple case: We have no mixers. Lets go on with next driver
			break;
		    }
		    else if ( false /* no multi-driver for now on applet !! m_multiDriverMode */ ) {
			// Special case: Multi-driver mode will probe more soundcards
			break;
		    }
		    else {
			// We have mixers, but no Multi-driver mode: Fine, we're done
			autodetectionFinished = true;
			break;
		    }
		}

		if ( mixer != 0 ) {
		    s_mixers->append( mixer );
		}

		// append driverName (used drivers)
		if ( !drvInfoAppended )
		{
		    drvInfoAppended = true;
		    QString driverName = Mixer::driverName(drv);
		    if ( drv!= 0 )
		    {
			driverInfoUsed += " + ";
		    }
		    driverInfoUsed += driverName;
		}

		// Check whether there are mixers in different drivers, so that the user can be warned
		if (!multipleDriversActive)
		{
		    if ( driverWithMixer == -1 )
		    {
			// Aha, this is the very first detected device
			driverWithMixer = drv;
		    }
		    else
		    {
			if ( driverWithMixer != drv )
			{
			    // Got him: There are mixers in different drivers
			    multipleDriversActive = true;
			}
		    }
		}

		// count mixer nums for every mixer name to identify mixers with equal names
		mixerNums[mixer->mixerName()]++;
		mixer->setMixerNum( mixerNums[mixer->mixerName()] );
	    } // loop over sound card devices of current driver
	} // loop over soundcard drivers

	m_hwInfoString = i18n("Sound drivers supported");
	m_hwInfoString += ": " + driverInfo +
		"\n" + i18n("Sound drivers used") + ": " + driverInfoUsed;
	if ( multipleDriversActive )
	{
		// this will only be possible by hacking the config-file, as it will not be officially supported
		m_hwInfoString += "\nExperimental multiple-Driver mode activated";
	}

	kdDebug(67100) << m_hwInfoString << endl;
    }	
    s_instCount++;
	
    KGlobal::dirs()->addResourceType( "appicon", KStandardDirs::kde_default("data") + "kmix/pics" );
	
    // get configuration
    KConfig *cfg = kmixApp->config();
    cfg->setGroup(0);
	
    // get colors
    _customColors = cfg->readBoolEntry( "ColorCustom", false );
	
    m_colors.high = cfg->readColorEntry("ColorHigh", &highColor);
    m_colors.low = cfg->readColorEntry("ColorLow", &lowColor);
    m_colors.back = cfg->readColorEntry("ColorBack", &backColor);

    m_colors.mutedHigh = cfg->readColorEntry("MutedColorHigh", &mutedHighColor);
    m_colors.mutedLow = cfg->readColorEntry("MutedColorLow", &mutedLowColor);
    m_colors.mutedBack = cfg->readColorEntry("MutedColorBack", &mutedBackColor);

    // find out to use which mixer
    mixerNum = cfg->readNumEntry( "Mixer", -1 );
    const QString& mixerName = cfg->readEntry( "MixerName", QString::null );
    _mixer = 0;
    if ( mixerNum>=0 ) {
	for (_mixer=s_mixers->first(); _mixer!=0; _mixer=s_mixers->next())
	{
	    if ( _mixer->mixerName()==mixerName && _mixer->mixerNum()==mixerNum ) break;
	}
    }
	
    // don't prompt for a mixer if there is just one available
    if ( !_mixer && s_mixers->count() == 1 ) {
	_mixer = s_mixers->first();
    }
	
    //  Find out wether the applet should be reversed
    reversedDir = cfg->readBoolEntry("ReversedDirection", false);

    if ( !_mixer )
    {
	m_errorLabel = new QPushButton( i18n("Select Mixer"), this );
	m_errorLabel->setGeometry(0, 0, m_errorLabel->sizeHint().width(),  m_errorLabel->sizeHint().height() );
	resize( m_errorLabel->sizeHint() );
	connect( m_errorLabel, SIGNAL(clicked()), this, SLOT(selectMixer()) );
    }
    else {
	// To take over reversedDir and (more important) to create the mixer widget
	positionChange(position());
    }
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
      delete s_mixers;
   }
}

void KMixApplet::saveConfig()
{
    if ( m_mixerWidget ) {
        KConfig *cfg = kmixApp->config();
        cfg->setGroup( 0 );
        cfg->writeEntry( "Mixer", _mixer->mixerNum() );
        cfg->writeEntry( "MixerName", _mixer->mixerName() );

        cfg->writeEntry( "ColorCustom", _customColors );

        cfg->writeEntry( "ColorHigh", m_colors.high.name() );
        cfg->writeEntry( "ColorLow", m_colors.low.name() );
        cfg->writeEntry( "ColorBack", m_colors.back.name() );

        cfg->writeEntry( "ColorMutedHigh", m_colors.mutedHigh.name() );
        cfg->writeEntry( "ColorMutedLow", m_colors.mutedLow.name() );
        cfg->writeEntry( "ColorMutedBack", m_colors.mutedBack.name() );

        cfg->writeEntry( "ReversedDirection", reversedDir );

        saveConfig( cfg, "Widget" );
        cfg->sync();
    }
}

void KMixApplet::loadConfig( KConfig *config, const QString &grp )
{
    KMixToolBox::loadConfig(m_mixerWidget->_mdws, config, grp, "PanelApplet" );
}

void KMixApplet::saveConfig( KConfig *config, const QString &grp )
{
    config->setGroup( grp );
    // Write mixer name. It cannot be changed in the Mixer instance, but we need a "PK" when restoring the config.
    config->writeEntry("Mixer_Name_Key", _mixer->mixerName());
    KMixToolBox::saveConfig(m_mixerWidget->_mdws, config, grp, "PanelApplet" );
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

	 _mixer = mixer;
	 // Create the ViewApplet by calling positionChange() ... :)
	 // To take over reversedDir and (more important) to create the mixer widget
	 // if necessary!
	 positionChange(position());
      }
   }
}

KMixApplet::Direction KMixApplet::getDirectionFromPositionHack(Position pos) const {
	/*
	Position hack: KMixApplet was using popupDirection() which is nonsense and now leeds to
	an unusable KMixApplet on vertical panels. As the "Direction" is used throughout
	KMix we now use position and do an ugly conversion.
	This is to be removed for KMix3.0 !!!
	*/
	KPanelApplet::Direction dir = KPanelApplet::Down; // Random default value
	switch ( pos ) {
		case KPanelApplet::pTop    : dir=KPanelApplet::Up; break;
		case KPanelApplet::pLeft   : dir=KPanelApplet::Left; break;
		case KPanelApplet::pRight  : dir=KPanelApplet::Left; break;
		case KPanelApplet::pBottom : dir=KPanelApplet::Up; break;
	}
	return dir;
}

/*
void KMixApplet::updat eLayou tNow()
{
   m_lockedLayout++;
   emit updat eLayout();
   // ugly hack to get the config saved somehow
   // !!! @todo Fix it somehow else. But it will be difficult with the panel applet
   saveConfig();
   m_lockedLayout--;
}
*/


void KMixApplet::resizeEvent(QResizeEvent *e)
{
    //kdDebug(67100) << "KMixApplet::resizeEvent(). New MDW is at " << e->size() << endl;
    if ( m_mixerWidget ) m_mixerWidget->resize( e->size().width(), e->size().height() );
    if ( m_errorLabel  ) m_errorLabel ->resize( e->size().width(), e->size().height() );
    KPanelApplet::resizeEvent( e );
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
    if ( !_customColors ) {
        KMixApplet::Colors cols;
        cols.high = highColor;
        cols.low = lowColor;
        cols.back = backColor;
        cols.mutedHigh = mutedHighColor;
        cols.mutedLow = mutedLowColor;
        cols.mutedBack = mutedBackColor;

        setColors( cols );
    } else
        setColors( m_colors );
}

void KMixApplet::positionChange(Position pos) {
    if (!m_errorLabel) {
	// do this only after we deleted the error label
	if (m_mixerWidget) {
	    saveConfig(); // save the applet before recreating it
	    delete m_mixerWidget;
	}
	Direction dir = getDirectionFromPositionHack(pos);
	m_mixerWidget = new ViewApplet( this, _mixer->name(), _mixer, dir );
	m_mixerWidget->createDeviceWidgets();
	
	loadConfig( config(), "Widget" );
	setColors();
	
	const QSize panelAppletConstrainedSize = sizeHint();
	m_mixerWidget->setGeometry( 0, 0, panelAppletConstrainedSize.width(), panelAppletConstrainedSize.height() );
	resize( panelAppletConstrainedSize.width(), panelAppletConstrainedSize.height() );
	setFixedSize(panelAppletConstrainedSize.width(), panelAppletConstrainedSize.height() );
	//kdDebug(67100) << "KMixApplet::positionChange(). New MDW is at " << panelAppletConstrainedSize << endl;
	m_mixerWidget->show();
	connect( _mixer, SIGNAL(newVolumeLevels()), m_mixerWidget, SLOT(refreshVolumeLevels()) );
    }
}

QSize KMixApplet::sizeHint() const {
    if ( m_errorLabel !=0 ) {
	return m_errorLabel->sizeHint();
    }
    else if (  m_mixerWidget != 0) {
	return  m_mixerWidget->sizeHint();
    }
    else {
	// During construction of m_mixerWidget or if something goes wrong:
	// Return something that should resemble our former sizeHint().
	return size();
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

        m_pref->setUseCustomColors( _customColors );
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
    _customColors = m_pref->useCustomColors();
    reversedDir = m_pref->reverseDirection();
    if (!m_mixerWidget)
        return;

    QSize si = m_mixerWidget->size();
    positionChange( position());
    if( position() == pTop || position() == pBottom )
        setIcons( si.height()>=32 );
    else
        setIcons( si.width()>=32 );

    m_mixerWidget->resize( si );
    setColors();
    saveConfig(); // might be saved twice: once 10 lines above canning  positionChange() and here again
}


void KMixApplet::setColors( const Colors &color )
{
    QPtrList<QWidget> &mdws = m_mixerWidget->_mdws;
    for ( QWidget* qmdw=mdws.first(); qmdw != 0; qmdw=mdws.next() ) {
	if ( qmdw->inherits("MixDeviceWidget") ) { // -<- temporary check. Later we *know* that it is correct
	    static_cast<MixDeviceWidget*>(qmdw)->setColors( color.high, color.low, color.back );
	    static_cast<MixDeviceWidget*>(qmdw)->setMutedColors( color.mutedHigh, color.mutedLow, color.mutedBack );
	}
    }
}

void KMixApplet::setIcons( bool on )
{
    if ( _iconsEnabled!=on )
    {
	// value was changed
	_iconsEnabled = on;
	KMixToolBox::setIcons(m_mixerWidget->_mdws, on);
    }
}

void KMixApplet::setLabels( bool on )
{
    if ( _labelsEnabled!=on ) {
	// value was changed
	_labelsEnabled = on;
	KMixToolBox::setLabels(m_mixerWidget->_mdws, on);
    }
}

void KMixApplet::setTicks( bool on )
{
    if ( _ticksEnabled!=on )
    {
	// value was changed
	_ticksEnabled = on;
	KMixToolBox::setTicks(m_mixerWidget->_mdws, on);
    }
}

#include "kmixapplet.moc"

