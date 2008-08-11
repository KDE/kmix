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

// // KMix
#include "colorwidget.h"
#include "mixertoolbox.h"
#include "kmixapplet.h"
#include "kmixtoolbox.h"
#include "mdwslider.h"
#include "mixdevicewidget.h"
#include "mixer.h"
#include "version.h"
#include "viewapplet.h"


extern "C"
{
  KDE_EXPORT KPanelApplet* init(QWidget *parent, const QString& configFile)
  {
     KGlobal::locale()->insertCatalogue("kmix");
     return new KMixApplet(configFile, KPanelApplet::Normal,
                           parent, "kmixapplet");
  }
}

int KMixApplet::s_instCount = 0;
//<Mixer> KMixApplet::Mixer::mixers();

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


KMixApplet::KMixApplet( const QString& configFile, Type t,
                        QWidget *parent, const char *name )

   : KPanelApplet( configFile, t, KPanelApplet::Preferences | KPanelApplet::ReportBug | KPanelApplet::About, parent, name ),
     m_mixerWidget(0), m_errorLabel(0), m_pref(0),
     m_aboutData( "kmix", I18N_NOOP("KMix Panel Applet"),
                         APP_VERSION, "Mini Sound Mixer Applet", KAboutData::License_GPL,
                         I18N_NOOP( "(c) 1996-2000 Christian Esken\n(c) 2000-2003 Christian Esken, Stefan Schimanski") )
{
    setBackgroundOrigin(AncestorOrigin);
    kdDebug(67100) << "KMixApplet::KMixApplet instancing Applet. Old s_instCount="<< s_instCount << " configfile=" << configFile << endl;
    //kdDebug(67100) << "KMixApplet::KMixApplet()" << endl;
    _layout = new QHBoxLayout(this); // it will always only be one item in it, so we don't care whether it is HBox or VBox

    // init static vars
    if ( s_instCount == 0) {
        Mixer::mixers().setAutoDelete( TRUE );
	QString dummyStringHwinfo;
	MixerToolBox::initMixer(Mixer::mixers(), false, dummyStringHwinfo);
    }	
    s_instCount++;
    kdDebug(67100) << "KMixApplet::KMixApplet instancing Applet, s_instCount="<< s_instCount << endl;
	
    KGlobal::dirs()->addResourceType( "appicon", KStandardDirs::kde_default("data") + "kmix/pics" );

    loadConfig();
	

    /********** find out to use which mixer ****************************************/
    _mixer = 0;
    for (_mixer= Mixer::mixers().first(); _mixer!=0; _mixer=Mixer::mixers().next())
    {
      if ( _mixer->id() == _mixerId ) break;
    }
    if ( _mixer == 0 ) {
      /* Until KMix V3.4-0 the mixerNumber (int) was stored. This was too complicated to handle, so we use an
       * unique ID (_mixer->mixerId(). But in case when the user changes soundcards (or when upgrading from
       * KMix 3.4-0 to a 3.4-1 or newer), we scan also for the soundcard name */
      for (_mixer= Mixer::mixers().first(); _mixer!=0; _mixer=Mixer::mixers().next())
      {
	if ( _mixer->mixerName() == _mixerName ) break;
      }
    }
	
    // don't prompt for a mixer if there is just one available
    if ( !_mixer && Mixer::mixers().count() == 1 ) {
	_mixer = Mixer::mixers().first();
    }
	


    if ( _mixer == 0 )
    {
	// No mixer set by user (kmixappletrc_*) and more than one to choose
	// We do NOT know which mixer to use => ask the User
	m_errorLabel = new QPushButton( i18n("Select Mixer"), this );
	m_errorLabel->setGeometry(0, 0, m_errorLabel->sizeHint().width(),  m_errorLabel->sizeHint().height() );
	resize( m_errorLabel->sizeHint() );
	connect( m_errorLabel, SIGNAL(clicked()), this, SLOT(selectMixer()) );
    }
    else {
	// We know which mixer to use: Call positionChange(), which does all the creating
	positionChange(position());
    }
    m_aboutData.addCredit( I18N_NOOP( "For detailed credits, please refer to the About information of the KMix program" ) );
}

KMixApplet::~KMixApplet()
{
   saveConfig();

   /* !!! no cleanup for now: I get strange crashes on exiting
   // destroy static vars
   s_instCount--;
   if ( s_instCount == 0)
   {
      MixerToolBox::deinitMixer();
   }
   */
}

void KMixApplet::saveConfig()
{
    kdDebug(67100) << "KMixApplet::saveConfig()" << endl;
    if ( m_mixerWidget != 0) {
	//kdDebug(67100) << "KMixApplet::saveConfig() save" << endl;
        KConfig *cfg = this->config();
	//kdDebug(67100) << "KMixApplet::saveConfig() save cfg=" << cfg << endl;
        cfg->setGroup( 0 );
        cfg->writeEntry( "Mixer", _mixer->id() );
        cfg->writeEntry( "MixerName", _mixer->mixerName() );

        cfg->writeEntry( "ColorCustom", _customColors );

        cfg->writeEntry( "ColorHigh", _colors.high.name() );
        cfg->writeEntry( "ColorLow", _colors.low.name() );
        cfg->writeEntry( "ColorBack", _colors.back.name() );

        cfg->writeEntry( "ColorMutedHigh", _colors.mutedHigh.name() );
        cfg->writeEntry( "ColorMutedLow", _colors.mutedLow.name() );
        cfg->writeEntry( "ColorMutedBack", _colors.mutedBack.name() );

        //cfg->writeEntry( "ReversedDirection", reversedDir );

        saveConfig( cfg, "Widget" );
        cfg->sync();
    }
}


void KMixApplet::loadConfig()
{
    kdDebug(67100) << "KMixApplet::loadConfig()" << endl;
    KConfig *cfg = this->config();
    cfg->setGroup(0);
	
    _mixerId = cfg->readEntry( "Mixer", "undef" );
    _mixerName = cfg->readEntry( "MixerName", QString::null );

    _customColors = cfg->readBoolEntry( "ColorCustom", false );
	
    _colors.high = cfg->readColorEntry("ColorHigh", &highColor);
    _colors.low = cfg->readColorEntry("ColorLow", &lowColor);
    _colors.back = cfg->readColorEntry("ColorBack", &backColor);

    _colors.mutedHigh = cfg->readColorEntry("ColorMutedHigh", &mutedHighColor);
    _colors.mutedLow = cfg->readColorEntry("ColorMutedLow", &mutedLowColor);
    _colors.mutedBack = cfg->readColorEntry("ColorMutedBack", &mutedBackColor);

    loadConfig( cfg, "Widget");
}


void KMixApplet::loadConfig( KConfig *config, const QString &grp )
{
    if ( m_mixerWidget ) {
	//config->setGroup( grp );
	KMixToolBox::loadConfig(m_mixerWidget->_mdws, config, grp, "PanelApplet" );
    }
}


void KMixApplet::saveConfig( KConfig *config, const QString &grp )
{
    if ( m_mixerWidget ) {
	config->setGroup( grp );
	// Write mixer name. It cannot be changed in the Mixer instance,
	// it is only saved for diagnostical purposes (analyzing the config file).
	config->writeEntry("Mixer_Name_Key", _mixer->mixerName());
	KMixToolBox::saveConfig(m_mixerWidget->_mdws, config, grp, "PanelApplet" );
    }
}

/**
 * Opens a dialog box with all available mixers and let the user choose one.
 * If the user selects a mixer, "_mixer" will be set and positionChange() is called.
 */
void KMixApplet::selectMixer()
{
   QStringList lst;

   int n=1;
   for (Mixer *mixer=Mixer::mixers().first(); mixer!=0; mixer=Mixer::mixers().next())
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
      Mixer *mixer = Mixer::mixers().at( lst.findIndex( res ) );
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


void KMixApplet::about()
{
    KAboutApplication aboutDlg(&m_aboutData);
    aboutDlg.exec();
}

void KMixApplet::help()
{
}


void KMixApplet::positionChange(Position pos) {
    orientationChange( orientation() );
    QResizeEvent e( size(), size() ); // from KPanelApplet::positionChange
    resizeEvent( &e ); // from KPanelApplet::positionChange
    
    if ( m_errorLabel == 0) {
	// do this only after we deleted the error label
	if (m_mixerWidget) {
	    saveConfig(); // save the applet before recreating it
	    _layout->remove(m_mixerWidget);
	    delete m_mixerWidget;
	}
	m_mixerWidget = new ViewApplet( this, _mixer->name(), _mixer, 0, pos );
	connect ( m_mixerWidget, SIGNAL(appletContentChanged()), this, SLOT(updateGeometrySlot()) );
	m_mixerWidget->createDeviceWidgets();
	_layout->add(m_mixerWidget);
	_layout->activate();
	
	loadConfig();
	setColors();
	
	const QSize panelAppletConstrainedSize = sizeHint();
	m_mixerWidget->setGeometry( 0, 0, panelAppletConstrainedSize.width(), panelAppletConstrainedSize.height() );
	resize( panelAppletConstrainedSize.width(), panelAppletConstrainedSize.height() );
	//setFixedSize(panelAppletConstrainedSize.width(), panelAppletConstrainedSize.height() );
	//kdDebug(67100) << "KMixApplet::positionChange(). New MDW is at " << panelAppletConstrainedSize << endl;
	m_mixerWidget->show();
	//connect( _mixer, SIGNAL(newVolumeLevels()), m_mixerWidget, SLOT(refreshVolumeLevels()) );
    }
}


/************* GEOMETRY STUFF START ********************************/
void KMixApplet::resizeEvent(QResizeEvent *e)
{
    //kdDebug(67100) << "KMixApplet::resizeEvent(). New MDW is at " << e->size() << endl;

    if ( position() == KPanelApplet::pLeft || position() == KPanelApplet::pRight ) {
        if ( m_mixerWidget ) m_mixerWidget->resize(e->size().width(),m_mixerWidget->height());
        if ( m_errorLabel  ) m_errorLabel ->resize(e->size().width(),m_errorLabel ->height());
    }
    else {
        if ( m_mixerWidget ) m_mixerWidget->resize(m_mixerWidget->width(), e->size().height());
        if ( m_errorLabel  ) m_errorLabel ->resize(m_errorLabel ->width() ,e->size().height());
    }


    // resizing changes our own sizeHint(), because we must take the new PanelSize in account.
    // So updateGeometry() is amust for us.
    //kdDebug(67100) << "KMixApplet::resizeEvent(). UPDATE GEOMETRY" << endl;
    updateGeometry();
    //kdDebug(67100) << "KMixApplet::resizeEvent(). EMIT UPDATE LAYOUT" << endl;
    emit updateLayout();
}

void KMixApplet::updateGeometrySlot() {
   updateGeometry();
}


QSize KMixApplet::sizeHint() const {
    //kdDebug(67100) << "KMixApplet::sizeHint()\n";
    QSize qsz;
    if ( m_errorLabel !=0 ) {
	qsz = m_errorLabel->sizeHint();
    }
    else if (  m_mixerWidget != 0) {
	qsz = m_mixerWidget->sizeHint();
    }
    else {
	// During construction of m_mixerWidget or if something goes wrong:
	// Return something that should resemble our former sizeHint().
	qsz = size();
    }
    //kdDebug(67100) << "KMixApplet::sizeHint() leftright =" << qsz << "\n";
    return qsz;
}

/**
   We need widthForHeight() and heigthForWidth() only because KPanelApplet::updateLayout does relayouts
   using this method. Actually we ignore the passed paramater and just return our preferred size.
*/
int KMixApplet::widthForHeight(int) const {
    //kdDebug(67100) << "KMixApplet::widthForHeight() = " << sizeHint().width() << endl;
    return sizeHint().width();
}
int KMixApplet::heightForWidth(int) const {
    //kdDebug(67100) << "KMixApplet::heightForWidth() = " << sizeHint().height() << endl;
    return sizeHint().height();
}




QSizePolicy KMixApplet::sizePolicy() const {
    //    return QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    if ( orientation() == Qt::Vertical ) {
	//kdDebug(67100) << "KMixApplet::sizePolicy=(Ignored,Fixed)\n";
        return QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
    else {
	//kdDebug(67100) << "KMixApplet::sizePolicy=(Fixed,Ignored)\n";
        return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
   }
}

/************* GEOMETRY STUFF END ********************************/


void KMixApplet::reportBug()
{
    KBugReport bugReportDlg(this, true, &m_aboutData);
    bugReportDlg.exec();
}


/******************* COLOR STUFF START ***********************************/

void KMixApplet::preferences()
{
    if ( !m_pref )
    {
        m_pref = new AppletConfigDialog( this );
        connect(m_pref, SIGNAL(finished()), SLOT(preferencesDone()));
        connect( m_pref, SIGNAL(applied()), SLOT(applyPreferences()) );

        m_pref->setActiveColors(_colors.high     , _colors.low     , _colors.back);
        m_pref->setMutedColors (_colors.mutedHigh, _colors.mutedLow, _colors.mutedBack);

        m_pref->setUseCustomColors( _customColors );

    }

    m_pref->show();
    m_pref->raise();
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

    // copy the colors from the prefs dialog
    m_pref->activeColors(_colors.high     , _colors.low     , _colors.back);
    m_pref->mutedColors (_colors.mutedHigh, _colors.mutedLow, _colors.mutedBack);
    _customColors = m_pref->useCustomColors();
    if (!m_mixerWidget)
        return;

    /*
    QSize si = m_mixerWidget->size();
    m_mixerWidget->resize( si );
    */
    setColors();
    saveConfig();
}

void KMixApplet::paletteChange ( const QPalette &) {
    if ( ! _customColors ) {
	// We take over Colors from paletteChange(), if the user has not set custom colors.
	// ignore the given QPalette and use the values from KGlobalSettings instead
	_colors.high = KGlobalSettings::highlightColor();
	_colors.low  = KGlobalSettings::baseColor();
	_colors.back = backColor;
	setColors( _colors );
    }
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
        setColors( _colors );
}

void KMixApplet::setColors( const Colors &color )
{
    if ( m_mixerWidget == 0 ) {
        // can happen for example after a paletteChange()
        return;
    }
    QPtrList<QWidget> &mdws = m_mixerWidget->_mdws;
    for ( QWidget* qmdw=mdws.first(); qmdw != 0; qmdw=mdws.next() ) {
	if ( qmdw->inherits("MixDeviceWidget") ) { // -<- temporary check. Later we *know* that it is correct
	    static_cast<MixDeviceWidget*>(qmdw)->setColors( color.high, color.low, color.back );
	    static_cast<MixDeviceWidget*>(qmdw)->setMutedColors( color.mutedHigh, color.mutedLow, color.mutedBack );
	}
    }
}

/******************* COLOR STUFF END ***********************************/

#include "kmixapplet.moc"

