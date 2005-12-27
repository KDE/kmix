/*
 * KMix -- KDE's full featured mini mixer
 *
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


// Qt
#include <qlabel.h>
#include <qlayout.h>
#include <qslider.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qapplication.h> // for QApplication::revsreseLayout()

// KDE
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <ktabwidget.h>

// KMix
#include "mixdevicewidget.h"
#include "kmixerwidget.h"
#include "kmixtoolbox.h"
#include "mixer.h"
#include "viewinput.h"
#include "viewoutput.h"
#include "viewswitches.h"
// KMix experimental
#include "viewgrid.h"
#include "viewsurround.h"


/**
   This widget is embedded in the KMix Main window. Each Hardware Mixer is visualized by one KMixerWidget.
   KMixerWidget contains
   (a) a headline where you can change Mixer's (if you got more than one Mixer)
   (b) a Tab with 2-4 Tabs (containing View's with sliders, switches and other GUI elements visualizing the Mixer)
   (c) A balancing slider
   (d) A label containg the mixer name
*/
KMixerWidget::KMixerWidget( int _id, Mixer *mixer, const QString &mixerName,
                            MixDevice::DeviceCategory categoryMask,
                            QWidget * parent, const char * name, ViewBase::ViewFlags vflags )
   : QWidget( parent, name ), _mixer(mixer), m_balanceSlider(0),
     m_topLayout(0),
     m_id( _id ),
     _iconsEnabled( true ), _labelsEnabled( false ), _ticksEnabled( false ),
     _valueStyle ( -1 ) // this definitely does not correspond to the 'default value display' style,
			// so the style will be set by a later call to setValueStyle()

{
    m_categoryMask = categoryMask;

   if ( _mixer )
   {
      createLayout(vflags);
   }
   else
   {
       // No mixer found
       // !! Fix this: This is actually never shown!
       QBoxLayout *layout = new QHBoxLayout( this );
       QString s = i18n("Invalid mixer");
       if ( !mixerName.isEmpty() )
	   s.append(" \"").append(mixerName).append("\"");
       QLabel *errorLabel = new QLabel( s, this );
       errorLabel->setAlignment( QLabel::AlignCenter | QLabel::WordBreak );
       layout->addWidget( errorLabel );
   }
}

KMixerWidget::~KMixerWidget()
{
}

/**
 * Creates the widgets as described in the KMixerWidget constructor
 */
void KMixerWidget::createLayout(ViewBase::ViewFlags vflags)
{
    // delete old objects
    if( m_balanceSlider ) {
	delete m_balanceSlider;
    }
    if( m_topLayout ) {
	delete m_topLayout;
    }

    // create main layout
    m_topLayout = new QVBoxLayout( this, 0, 3,  "m_topLayout" );

    // Create tabs of input + output + [...]
    m_ioTab = new KTabWidget( this, "ioTab" );
    m_topLayout->add( m_ioTab );


     /*******************************************************************
      *  Now the main GUI is created.
      * 1) Select a (GUI) profile,  which defines  which controls to show on which Tab
      * 2a) Create the Tab's and the corresponding Views
      * 2b) Create device widgets
      * 2c) Add Views to Tab
      ********************************************************************/
      //KMixGUIProfile* prof = MixerToolbox::selectProfile(_mixer);
      
      
	possiblyAddView(new ViewOutput  ( m_ioTab, "output", i18n("Output"), _mixer, vflags ) );
	possiblyAddView(new ViewInput( m_ioTab, "input", i18n("Input"), _mixer, vflags ) );
	possiblyAddView(new ViewSwitches( m_ioTab, "switches", i18n("Switches"), _mixer, vflags ) );
	if ( vflags & ViewBase::Experimental_SurroundView )
		possiblyAddView( new ViewSurround( m_ioTab, "surround", i18n("Surround"), _mixer, vflags ) );
    if ( vflags & ViewBase::Experimental_GridView )
		possiblyAddView( new ViewGrid( m_ioTab, "grid", i18n("Grid"), _mixer, vflags ) );


    // *** Lower part: Slider and Mixer Name ************************************************
    QHBoxLayout *balanceAndDetail = new QHBoxLayout( m_topLayout, 8,  "balanceAndDetail");
    // Create the left-right-slider
    m_balanceSlider = new QSlider( -100, 100, 25, 0, QSlider::Horizontal, this, "RightLeft" );
    m_balanceSlider->setTickmarks( QSlider::Below );
    m_balanceSlider->setTickInterval( 25 );
    m_balanceSlider->setMinimumSize( m_balanceSlider->sizeHint() );
    m_balanceSlider->setFixedHeight( m_balanceSlider->sizeHint().height() );

    QLabel *mixerName = new QLabel(this, "mixerName");
    mixerName->setText( _mixer->mixerName() );

    balanceAndDetail->addSpacing( 10 );

    balanceAndDetail->addWidget( m_balanceSlider );
    balanceAndDetail->addWidget( mixerName );
    balanceAndDetail->addSpacing( 10 );

    connect( m_balanceSlider, SIGNAL(valueChanged(int)), this, SLOT(balanceChanged(int)) );
    QToolTip::add( m_balanceSlider, i18n("Left/Right balancing") );

    // --- "MenuBar" toggling from the various View's ---



    show();
    //    kdDebug(67100) << "KMixerWidget::createLayout(): EXIT\n";
}

void KMixerWidget::possiblyAddView(ViewBase* vbase)
{
	if ( vbase->count() == 0 )
		delete vbase;
	else {
		_views.push_back(vbase);
		vbase ->createDeviceWidgets();
		m_ioTab->addTab( vbase , vbase->caption() );
		connect( vbase, SIGNAL(toggleMenuBar()), parentWidget(), SLOT(toggleMenuBar()) );
	}
}

void KMixerWidget::setIcons( bool on )
{
	for ( std::vector<ViewBase*>::iterator it = _views.begin(); it != _views.end();  it++) {
		ViewBase* mixerWidget = *it;
		KMixToolBox::setIcons(mixerWidget->_mdws, on);
    } // for all tabs
}

void KMixerWidget::setLabels( bool on )
{
    if ( _labelsEnabled!=on ) {
		// value was changed
		_labelsEnabled = on;
		for ( std::vector<ViewBase*>::iterator it = _views.begin(); it != _views.end();  it++) {
			ViewBase* mixerWidget = *it;
			KMixToolBox::setLabels(mixerWidget->_mdws, on);
	    } // for all tabs
    }
}

void KMixerWidget::setTicks( bool on )
{
    if ( _ticksEnabled!=on ) {
		// value was changed
		_ticksEnabled = on;
		for ( std::vector<ViewBase*>::iterator it = _views.begin(); it != _views.end();  it++) {
			ViewBase* mixerWidget = *it;
		    KMixToolBox::setTicks(mixerWidget->_mdws, on);
		} // for all tabs
    }
}

void KMixerWidget::setValueStyle( int vs )
{
    if ( _valueStyle!=vs ) {
		// value was changed
		_valueStyle = vs;
		for ( std::vector<ViewBase*>::iterator it = _views.begin(); it != _views.end();  it++) {
			ViewBase* mixerWidget = *it;
		    KMixToolBox::setValueStyle(mixerWidget->_mdws, vs);
		} // for all tabs
    }
}


/**
 *  @todo : Is the view list already filled, when loadConfig() is called?
 */
void KMixerWidget::loadConfig( KConfig *config, const QString &grp )
{

	for ( std::vector<ViewBase*>::iterator it = _views.begin(); it != _views.end();  it++) {
		ViewBase* mixerWidget = *it;
		QString viewPrefix = "View.";
		viewPrefix += mixerWidget->name();
		KMixToolBox::loadConfig(mixerWidget->_mdws, config, grp, viewPrefix );
		mixerWidget->configurationUpdate();
	} // for all tabs
}



void KMixerWidget::saveConfig( KConfig *config, const QString &grp )
{
	config->setGroup( grp );
	// Write mixer name. It cannot be changed in the Mixer instance,
	// it is only saved for diagnostical purposes (analyzing the config file).
	config->writeEntry("Mixer_Name_Key", _mixer->mixerName());
	
	for ( std::vector<ViewBase*>::iterator it = _views.begin(); it != _views.end();  it++) {
		ViewBase* mixerWidget = *it;
		QString viewPrefix = "View.";
		viewPrefix += mixerWidget->name();
		KMixToolBox::saveConfig(mixerWidget->_mdws, config, grp, viewPrefix );
	} // for all tabs
}


void KMixerWidget::toggleMenuBarSlot() {
    emit toggleMenuBar();
}

// in RTL mode, the slider is reversed, we cannot just connect the signal to setBalance()
// hack arround it before calling _mixer->setBalance()
void KMixerWidget::balanceChanged(int balance)
{
    if (QApplication::reverseLayout())
        balance = -balance;

    _mixer->setBalance( balance );
}

#include "kmixerwidget.moc"
