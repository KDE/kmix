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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// System
#include <stdlib.h>

// QT
#include <qgroupbox.h>
#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <qpixmap.h>
#include <QPushButton>
#include <qradiobutton.h>
#include <QResizeEvent>
#include <qwmatrix.h>


// KDE
#include <kaboutapplicationdialog.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kapplication.h>
#include <kbugreport.h>
#include <kcolorbutton.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kglobalaccel.h>
#include <kcolorscheme.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

// // KMix
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
  KDE_EXPORT K3PanelApplet* init(QWidget *parent, const QString& configFile)
  {
     KGlobal::locale()->insertCatalog("kmix");
     return new KMixApplet(configFile, K3PanelApplet::Normal,
                           parent, "kmixapplet");
  }
}

int KMixApplet::s_instCount = 0;
//<Mixer> KMixApplet::Mixer::mixers();

static const QColor highColor = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
static const QColor lowColor = KColorScheme(QPalette::Active, KColorScheme::Selection).background().color();
static const QColor backColor = "#000000";
static const QColor mutedHighColor = "#FFFFFF";
static const QColor mutedLowColor = "#808080";
static const QColor mutedBackColor = "#000000";

AppletConfigDialog::AppletConfigDialog( QWidget * parent, const char * name )
   : KDialog( parent)
{
   setObjectName(name);
   setButtons( KDialog::Ok | KDialog::Apply | KDialog::Cancel );
   setDefaultButton( Ok );
   setModal( false );
   showButtonSeparator( true );
   setPlainCaption(i18n("Configure - Mixer Applet"));
   QFrame* page = new QFrame(this);
   setMainWidget( page );
   QVBoxLayout *topLayout = new QVBoxLayout(page);
   colorWidget = new ColorWidget(page);
   topLayout->addWidget(colorWidget);
   setUseCustomColors(false);
   connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
   connect(this,SIGNAL(applyClicked()),this,SLOT(slotApply()));
}

void AppletConfigDialog::slotOk()
{
    slotApply();
    KDialog::accept();
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


KMixApplet::KMixApplet( const QString& configFile, K3PanelApplet::Type t,
                        QWidget *parent, const char *name )

   : K3PanelApplet( configFile, t, K3PanelApplet::Preferences | K3PanelApplet::ReportBug | K3PanelApplet::About, parent ),
     m_appletView(0), m_errorLabel(0), m_pref(0),
     m_aboutData( "kmix", 0, ki18n("KMix Panel Applet"),
                         APP_VERSION, ki18n("Mini Sound Mixer Applet"), KAboutData::License_GPL,
                         ki18n( "(c) 1996-2000 Christian Esken\n(c) 2000-2003 Christian Esken, Stefan Schimanski") )
{
   setObjectName(name);
    kDebug(67100) << "KMixApplet::KMixApplet instancing Applet. Old s_instCount="<< s_instCount << " configfile=" << configFile;
    //kDebug(67100) << "KMixApplet::KMixApplet()";
    _layout = new QHBoxLayout(this); // it will always only be one item in it, so we don't care whether it is HBox or VBox

    // init static vars
    if ( s_instCount == 0) {
        //  !!! TODO Mixer::mixers().setAutoDelete( true );
	QString dummyStringHwinfo;
	MixerToolBox::instance()->initMixer(false, dummyStringHwinfo);
    }
    s_instCount++;
    kDebug(67100) << "KMixApplet::KMixApplet instancing Applet, s_instCount="<< s_instCount;

    KGlobal::dirs()->addResourceType( "appicon", "data", "kmix/pics" );

    loadConfig();


    /********** find out to use which mixer ****************************************/
    _mixer = 0;
    for ( int i=0; i< Mixer::mixers().count(); ++i )
    {
       Mixer *mixer = (Mixer::mixers())[i];
       if ( mixer->id() == _mixerName ) {  // @todo Check the change: _mixer->  to   mixer->
          _mixer = mixer;
          break;
       }
    }

    if ( _mixer==0 && Mixer::mixers().count() == 1 ) {
        // No mixer was matching. But we still don't have to prompt the user, if
        // there is card available.
	_mixer = (Mixer::mixers())[0];
    }


    /*** Check, whether a Mixer could be selected automagically ******************/
    if ( _mixer == 0 )
    {
	// No mixer set by user (kmixappletrc_*) and more than one to choose
	// We do NOT know which mixer to use => Ask the User
	m_errorLabel = new QPushButton( i18n("Select Mixer"), this );
	m_errorLabel->setGeometry(0, 0, m_errorLabel->sizeHint().width(),  m_errorLabel->sizeHint().height() );
	resize( m_errorLabel->sizeHint() );
	connect( m_errorLabel, SIGNAL(clicked()), this, SLOT(selectMixer()) );
    }
    else {
	// We know which mixer to use: Call positionChange(), which does all the creating
	positionChange(position());
    }
    m_aboutData.addCredit( ki18n( "For detailed credits, please refer to the About information of the KMix program" ) );
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
    kDebug(67100) << "KMixApplet::saveConfig()";
    if ( m_appletView != 0) {
	//kDebug(67100) << "KMixApplet::saveConfig() save";
	//kDebug(67100) << "KMixApplet::saveConfig() save cfg=" << cfg;
        KConfigGroup grp( this->config(), "<default>");
        grp.writeEntry( "Mixer", _mixer->id() );
        grp.writeEntry( "MixerName", _mixer->id() );

        grp.writeEntry( "ColorCustom", _customColors );

        grp.writeEntry( "ColorHigh", _colors.high.name() );
        grp.writeEntry( "ColorLow", _colors.low.name() );
        grp.writeEntry( "ColorBack", _colors.back.name() );

        grp.writeEntry( "ColorMutedHigh", _colors.mutedHigh.name() );
        grp.writeEntry( "ColorMutedLow", _colors.mutedLow.name() );
        grp.writeEntry( "ColorMutedBack", _colors.mutedBack.name() );

        //cfg->writeEntry( "ReversedDirection", reversedDir );

        saveConfig( config(), "Widget" );
        config()->sync();
    }
}


void KMixApplet::loadConfig()
{
    kDebug(67100) << "KMixApplet::loadConfig()";
    KConfigGroup cfg(this->config(), "<default>");

    _mixerId = cfg.readEntry( "Mixer", "undef" );
    _mixerName = cfg.readEntry( "MixerName", QString());

    _customColors = cfg.readEntry( "ColorCustom", false );

    _colors.high = cfg.readEntry("ColorHigh", highColor);
    _colors.low = cfg.readEntry("ColorLow", lowColor);
    _colors.back = cfg.readEntry("ColorBack", backColor);

    _colors.mutedHigh = cfg.readEntry("ColorMutedHigh", mutedHighColor);
    _colors.mutedLow = cfg.readEntry("ColorMutedLow", mutedLowColor);
    _colors.mutedBack = cfg.readEntry("ColorMutedBack", mutedBackColor);

    loadConfig( this->config(), "Widget");
}


void KMixApplet::loadConfig( KConfig *config, const QString& /*grp*/ )
{
    if ( m_appletView ) {
	KMixToolBox::loadView(m_appletView, config );
	KMixToolBox::loadKeys(m_appletView, config );
    }
}


void KMixApplet::saveConfig( KConfig *config, const QString &grp )
{
    if ( m_appletView ) {
	// Write mixer name. It cannot be changed in the Mixer instance,
	// it is only saved for diagnostical purposes (analyzing the config file).
	KConfigGroup cg(config, grp );
	cg.writeEntry("Mixer_Name_Key", _mixer->id());

	KMixToolBox::saveView(m_appletView, config );
	KMixToolBox::saveKeys(m_appletView, config );
    }
}

/**
 * Opens a dialog box with all available mixers and let the user choose one.
 * If the user selects a mixer, "_mixer" will be set and positionChange() is called.
 */
void KMixApplet::selectMixer()
{
   QStringList lst;

   for ( int i=0; i< Mixer::mixers().count(); ++i )
   {
       Mixer *mixer = (Mixer::mixers())[i];
       QString s;
       s.sprintf("%i. %s", (i+1), mixer->id().toAscii().data());
       lst << s;
   }

   bool ok = false;
   QString res = KInputDialog::getItem( i18n("Mixers"),
                                        i18n("Available mixers:"),
					lst, 1, false, &ok, this );
   if ( ok )
   {
      Mixer *mixer = Mixer::mixers().at( lst.indexOf( res ) );
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
    KAboutApplicationDialog aboutDlg(&m_aboutData);
    aboutDlg.exec();
}

void KMixApplet::help()
{
}


void KMixApplet::positionChange(K3PanelApplet::Position pos) {
    orientationChange( orientation() );
    QResizeEvent e( size(), size() ); // from K3PanelApplet::positionChange
    resizeEvent( &e ); // from K3PanelApplet::positionChange

    if ( m_errorLabel == 0) {
	// do this only after we deleted the error label
	if (m_appletView) {
	    saveConfig(); // save the applet before recreating it
	    _layout->removeWidget(m_appletView);
	    delete m_appletView;
	}
 	/**@todo Add View stuff to KMixApplet / ViewApplet */
	m_appletView = new ViewApplet( this, _mixer->id().toAscii().data(), _mixer, 0,
                (GUIProfile*)0, pos );
	connect ( m_appletView, SIGNAL(appletContentChanged()), this, SLOT(updateGeometrySlot()) );
	m_appletView->createDeviceWidgets();
	_layout->addWidget(m_appletView);
	_layout->activate();

	loadConfig();
	setColors();

	const QSize panelAppletConstrainedSize = sizeHint();
	m_appletView->setGeometry( 0, 0, panelAppletConstrainedSize.width(), panelAppletConstrainedSize.height() );
	resize( panelAppletConstrainedSize.width(), panelAppletConstrainedSize.height() );
	//setFixedSize(panelAppletConstrainedSize.width(), panelAppletConstrainedSize.height() );
	//kDebug(67100) << "KMixApplet::positionChange(). New MDW is at " << panelAppletConstrainedSize;
	m_appletView->show();
	//connect( _mixer, SIGNAL(controlChanged()), m_appletView, SLOT(refreshVolumeLevels()) );
    }
}


/************* GEOMETRY STUFF START ********************************/
void KMixApplet::resizeEvent(QResizeEvent *e)
{
    //kDebug(67100) << "KMixApplet::resizeEvent(). New MDW is at " << e->size();

    if ( position() == K3PanelApplet::Left || position() == K3PanelApplet::Right ) {
        if ( m_appletView ) m_appletView->resize(e->size().width(),m_appletView->height());
        if ( m_errorLabel  ) m_errorLabel ->resize(e->size().width(),m_errorLabel ->height());
    }
    else {
        if ( m_appletView ) m_appletView->resize(m_appletView->width(), e->size().height());
        if ( m_errorLabel  ) m_errorLabel ->resize(m_errorLabel ->width() ,e->size().height());
    }


    // resizing changes our own sizeHint(), because we must take the new PanelSize in account.
    // So updateGeometry() is amust for us.
    //kDebug(67100) << "KMixApplet::resizeEvent(). UPDATE GEOMETRY";
    updateGeometry();
    //kDebug(67100) << "KMixApplet::resizeEvent(). EMIT UPDATE LAYOUT";
    emit updateLayout();
}

void KMixApplet::updateGeometrySlot() {
   updateGeometry();
}


QSize KMixApplet::sizeHint() const {
    //kDebug(67100) << "KMixApplet::sizeHint()\n";
    QSize qsz;
    if ( m_errorLabel !=0 ) {
	qsz = m_errorLabel->sizeHint();
    }
    else if (  m_appletView != 0) {
	qsz = m_appletView->sizeHint();
    }
    else {
	// During construction of m_appletView or if something goes wrong:
	// Return something that should resemble our former sizeHint().
	qsz = size();
    }
    //kDebug(67100) << "KMixApplet::sizeHint() leftright =" << qsz << "\n";
    return qsz;
}

/**
   We need widthForHeight() and heigthForWidth() only because K3PanelApplet::updateLayout does relayouts
   using this method. Actually we ignore the passed paramater and just return our preferred size.
*/
int KMixApplet::widthForHeight(int) const {
    //kDebug(67100) << "KMixApplet::widthForHeight() = " << sizeHint().width();
    return sizeHint().width();
}
int KMixApplet::heightForWidth(int) const {
    //kDebug(67100) << "KMixApplet::heightForWidth() = " << sizeHint().height();
    return sizeHint().height();
}




QSizePolicy KMixApplet::sizePolicy() const {
    //    return QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    if ( orientation() == Qt::Vertical ) {
	//kDebug(67100) << "KMixApplet::sizePolicy=(Ignored,Fixed)\n";
        return QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
    else {
	//kDebug(67100) << "KMixApplet::sizePolicy=(Fixed,Ignored)\n";
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
    if (!m_appletView)
        return;

    /*
    QSize si = m_appletView->size();
    m_appletView->resize( si );
    */
    setColors();
    saveConfig();
}

void KMixApplet::paletteChange ( const QPalette &) {
    if ( ! _customColors ) {
	// We take over Colors from paletteChange(), if the user has not set custom colors.
	// ignore the given QPalette and use the values from KColorScheme instead
	_colors.high = KColorScheme(QPalette::Active, KColorScheme::Selection).background().color();
	_colors.low  = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
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
    if ( m_appletView == 0 ) {
        // can happen for example after a paletteChange()
        return;
    }
    QList<QWidget *> &mdws = m_appletView->_mdws;
    for ( int i=0; i < mdws.count(); ++i ) {
        QWidget* qmdw = mdws[i];
	if ( qmdw->inherits("MixDeviceWidget") ) { // -<- temporary check. Later we *know* that it is correct
	    static_cast<MixDeviceWidget*>(qmdw)->setColors( color.high, color.low, color.back );
	    static_cast<MixDeviceWidget*>(qmdw)->setMutedColors( color.mutedHigh, color.mutedLow, color.mutedBack );
	}
    }
}

/******************* COLOR STUFF END ***********************************/

#include "kmixapplet.moc"

