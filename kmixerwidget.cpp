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
#include "viewsurround.h"


/**
   This widget is embedded in the KMix Main window. Each Hardware Mixer is visualized by one KMixerWidget.
   KMixerWidget contains
   (a) a headline where you can change Mixer's (if you got more than one Mixer)
   (b) a Tab with 2-4 Tabs (containing View's with sliders, switches and other GUI elements visualizing the Mixer)
   (c) A balancing slider
   (d) A label containg the mixer name
*/
KMixerWidget::KMixerWidget( int _id, Mixer *mixer, const QString &mixerName, int mixerNum,
                            MixDevice::DeviceCategory categoryMask,
                            QWidget * parent, const char * name, ViewBase::ViewFlags vflags )
   : QWidget( parent, name ), _mixer(mixer), m_balanceSlider(0),
     m_topLayout(0),
     //m_name( mixerName ), m_mixerName( mixerName ),
     m_mixerNum( mixerNum ), m_id( _id ),
     _iconsEnabled( true ), _labelsEnabled( false ), _ticksEnabled( false )

{
    m_categoryMask = categoryMask;

   _oWidget = 0;
   _iWidget = 0;
   _swWidget = 0;
   // Create mixer device widgets

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
	   s += " \"" + mixerName + "\"";
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

    // *** Create Views *********************************************************************
    _oWidget  = new ViewOutput  ( m_ioTab, "OutputTab" , _mixer, vflags );
    _iWidget  = new ViewInput   ( m_ioTab, "InputTab"  , _mixer, vflags );
    _swWidget = new ViewSwitches( m_ioTab, "SwitchTab" , _mixer, vflags );
    if ( vflags & ViewBase::Experimental_SurroundView )
	_surroundWidget = new ViewSurround( m_ioTab, "SurroundTab", _mixer, vflags );

    // *** Create device widgets ************************************************************
    _oWidget ->createDeviceWidgets();
    _iWidget ->createDeviceWidgets();
    _swWidget->createDeviceWidgets();
    if ( vflags & ViewBase::Experimental_SurroundView )
	_surroundWidget->createDeviceWidgets();

    // *** Add Views to Tab *****************************************************************
    m_ioTab->addTab( _oWidget , i18n("Output") );
    m_ioTab->addTab( _iWidget , i18n("Input" ) );
    if ( _swWidget->count() > 0 ) { // Add switches Tab only, if there are Switches
	m_ioTab->addTab( _swWidget, i18n("Switches" ) );
    }
    else {
	delete _swWidget;
	_swWidget = 0;
    }
    if ( vflags & ViewBase::Experimental_SurroundView )
	m_ioTab->addTab(_surroundWidget, i18n("Surround" ) );


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
    connect( _oWidget, SIGNAL(toggleMenuBar()), parentWidget(), SLOT(toggleMenuBar()) );
    connect( _iWidget, SIGNAL(toggleMenuBar()), parentWidget(), SLOT(toggleMenuBar()) );
    if (_swWidget != 0 )
	connect( _swWidget, SIGNAL(toggleMenuBar()), parentWidget(), SLOT(toggleMenuBar()) );
    if ( vflags & ViewBase::Experimental_SurroundView )
	connect( _surroundWidget, SIGNAL(toggleMenuBar()), parentWidget(), SLOT(toggleMenuBar()) );


    show();
    //    kdDebug(67100) << "KMixerWidget::createLayout(): EXIT\n";
}


void KMixerWidget::setIcons( bool on )
{
    for (int i=0; i<=2; i++) {
	ViewBase* m_mixerWidget;
	switch (i) {
	case 0: m_mixerWidget =  _oWidget; break;
	case 1: m_mixerWidget =  _iWidget; break;
	case 2: if ( _swWidget == 0) continue; else m_mixerWidget = _swWidget; break;
	default: kdError(67100) << "KMixerWidget::setIcons(): wrong _mixerWidget " << i << "\n";; continue;
	}

	KMixToolBox::setIcons(m_mixerWidget->_mdws, on);
    }
}

void KMixerWidget::setLabels( bool on )
{
    if ( _labelsEnabled!=on ) {
	// value was changed
	_labelsEnabled = on;
	for (int i=0; i<=2; i++) {
	    ViewBase* m_mixerWidget;
	    switch (i) {
	    case 0: m_mixerWidget =  _oWidget; break;
	    case 1: m_mixerWidget =  _iWidget; break;
	    case 2: if ( _swWidget == 0) continue; else m_mixerWidget = _swWidget; break;
	    default: kdError(67100) << "KMixerWidget::setLabels(): wrong _mixerWidget " << i << "\n"; continue;
	    }

	    KMixToolBox::setLabels(m_mixerWidget->_mdws, on);
	}
    }
}

void KMixerWidget::setTicks( bool on )
{
    if ( _ticksEnabled!=on ) {
	// value was changed
	_ticksEnabled = on;
	for (int i=0; i<=2; i++) {
	    ViewBase* m_mixerWidget;
	    switch (i) {
	    case 0: m_mixerWidget =  _oWidget; break;
	    case 1: m_mixerWidget =  _iWidget; break;
	    case 2: if ( _swWidget == 0) continue; else m_mixerWidget = _swWidget; break;
	    default: kdError(67100) << "KMixerWidget::setTicks(): wrong _mixerWidget " << i << "\n"; continue;
	    }

	    KMixToolBox::setTicks(m_mixerWidget->_mdws, on);
	}
    }
}


void
KMixerWidget::loadConfig( KConfig *config, const QString &grp )
{
  for (int i=0; i<=2; i++) {
      ViewBase* m_mixerWidget;
      QString viewPrefix = "View.";
      switch (i) {
      case 0: m_mixerWidget =  _oWidget; viewPrefix += "Output"   ; break;
      case 1: m_mixerWidget =  _iWidget; viewPrefix += "Input"    ; break;
      case 2: if ( _swWidget == 0) continue; else m_mixerWidget = _swWidget; viewPrefix += "Switches" ; break;
      default: kdError(67100) << "KMixerWidget::loadConfig(): wrong _mixerWidget " << i << "\n"; continue;
      }

      KMixToolBox::loadConfig(m_mixerWidget->_mdws, config, grp, viewPrefix );
      m_mixerWidget->configurationUpdate();
  } // for the 3 tabs
}



void KMixerWidget::saveConfig( KConfig *config, const QString &grp )
{
    config->setGroup( grp );
    // Write mixer name. It cannot be changed in the Mixer instance,
    // it is only saved for diagnostical purposes (analyzing the config file).
    config->writeEntry("Mixer_Name_Key", _mixer->mixerName());

    for (int i=0; i<=2; i++) {
	ViewBase* m_mixerWidget;
	QString viewPrefix = "View.";
	switch (i) {
	case 0: m_mixerWidget =  _oWidget; viewPrefix += "Output"   ; break;
	case 1: m_mixerWidget =  _iWidget; viewPrefix += "Input"    ; break;
	case 2: if ( _swWidget == 0) continue; else m_mixerWidget = _swWidget; viewPrefix += "Switches" ; break;
	default: kdError(67100) << "KMixerWidget::saveConfig(): wrong _mixerWidget " << i << "\n"; continue;
	}

	KMixToolBox::saveConfig(m_mixerWidget->_mdws, config, grp, viewPrefix );
    } // for the 3 tabs
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
