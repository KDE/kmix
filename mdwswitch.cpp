/*
 * KMix -- KDE's full featured mini mixer
 *
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

#include <qcursor.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qobject.h>
#include <qslider.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kglobalaccel.h>
#include <kkeydialog.h>
#include <kdebug.h>

#include "kledbutton.h"
#include "mdwswitch.h"
#include "mixer.h"
#include "viewbase.h"
#include "verticaltext.h"

/**
 * Class that represents a single Switch
 * The direction (horizontal, vertical) can be configured and whether it should
 * be "small"  (uses KSmallSlider instead of QSlider then).
 */
MDWSwitch::MDWSwitch(Mixer *mixer, MixDevice* md,
                                 bool small, KPanelApplet::Direction dir,
                                 QWidget* parent, ViewBase* mw, const char* name) :
    MixDeviceWidget(mixer,md,small,dir,parent,mw,name),
    _label(0) , _labelV(0) , _switchLED(0), _layout(0)
{
    // create actions (on _mdwActions, see MixDeviceWidget)

    // KStdAction::showMenubar() is in MixDeviceWidget now
    new KToggleAction( i18n("&Hide"), 0, this, SLOT(setDisabled()), _mdwActions, "hide" );
    new KAction( i18n("Define &Keys..."), 0, this, SLOT(defineKeys()), _mdwActions, "keys" );

    // create widgets
    createWidgets();

    m_keys->insert( "Toggle switch", i18n( "Toggle Switch" ), QString::null,
		    KShortcut(), KShortcut(), this, SLOT( toggleSwitch() ) );

    // The keys are loaded in KMixerWidget::loadConfig, see kmixerwidget.cpp (now: kmixtoolbox.cpp)
    //m_keys->readSettings();
    //m_keys->updateConnections();

    installEventFilter( this ); // filter for popup
}

MDWSwitch::~MDWSwitch()
{
}


void MDWSwitch::createWidgets()
{
    if ( (m_direction == KPanelApplet::Up) ||  (m_direction == KPanelApplet::Down) ) {
	_layout = new QVBoxLayout( this );
	_layout->setAlignment(Qt::AlignHCenter);
    }
    else {
	_layout = new QHBoxLayout( this );
	_layout->setAlignment(Qt::AlignVCenter);
    }
    QToolTip::add( this, m_mixdevice->name() );


    _layout->addSpacing( 4 );
    // --- DEVICE ICON --------------------------
    if ((m_direction == KPanelApplet::Up) || (m_direction == KPanelApplet::Left))
    {
	_switchLED = new KLedButton( Qt::yellow, KLed::On, KLed::Sunken,
				     KLed::Circular, this, "SwitchLED" );
        _switchLED->setFixedSize(16,16);
	//_labelV = new VerticalText( this, m_mixdevice->name().latin1() );
	_labelV = new VerticalText( this, m_mixdevice->name().utf8().data() );

	_layout->addWidget( _switchLED );
	_layout->addSpacing( 2 );
	_layout->addWidget( _labelV );

	_switchLED->installEventFilter( this );
	_labelV->installEventFilter( this );
    }
    else
    {
	_switchLED = new KLedButton( Qt::yellow, KLed::On, KLed::Sunken,
				     KLed::Circular, this, "SwitchLED" );
        _switchLED->setFixedSize(16,16);
	_label  = new QLabel(m_mixdevice->name(), this, "SwitchName");

	_layout->addWidget( _switchLED );
	_layout->addSpacing( 1 );
	_layout->addWidget( _label );

	_switchLED->installEventFilter( this );
	_label->installEventFilter( this );
    }
    connect( _switchLED, SIGNAL(stateChanged(bool)), this, SLOT(toggleSwitch()) );
    _layout->addSpacing( 4 );
}

void MDWSwitch::update()
{
    if ( _switchLED != 0 ) {
	_switchLED->blockSignals( true );
	_switchLED->setState( m_mixdevice->isMuted() ? KLed::Off : KLed::On );
	_switchLED->blockSignals( false );
    }
}

void MDWSwitch::setBackgroundMode(BackgroundMode m)
{
    if ( _label != 0 ){
	_label->setBackgroundMode(m);
    }
    if ( _labelV != 0 ){
	_labelV->setBackgroundMode(m);
    }
    _switchLED->setBackgroundMode(m);
    MixDeviceWidget::setBackgroundMode(m);
}

void MDWSwitch::showContextMenu()
{
    if( m_mixerwidget == NULL )
	return;

    KPopupMenu *menu = m_mixerwidget->getPopup();

    /* !! do key later
    menu->insertTitle( m_mixdevice->name() );
    KAction *a = _mdwActions->action( "keys" );
    if ( a && m_keys )
    {
	KActionSeparator sep( this );
	sep.plug( menu );
	a->plug( menu );
    }
    */

    QPoint pos = QCursor::pos();
    menu->popup( pos );
}

QSize MDWSwitch::sizeHint() {
    if ( _layout != 0 ) {
	return _layout->sizeHint();
    }
    else {
	// layout not (yet) created
	return QWidget::sizeHint();
    }
}


/**
   This slot is called, when a user has clicked the mute button. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWSwitch::toggleSwitch() {
    setSwitch( !m_mixdevice->isMuted() );
}

void MDWSwitch::setSwitch(bool value)
{
    if (  m_mixdevice->isSwitch() ) {
	// Attention: Please note that we are using the "muted" flag
	//            This has reversed semantics:
	// muted : 1=off (no sound)
	// switch: 1=on  (enabled)
	// 
	// But we still don't use "!value", because actually we only toggle the switch:
	// In other words "This method is only called from MDWSwitch::toggleSwitch()"
	m_mixdevice->setMuted( value );
	m_mixer->commitVolumeChange(m_mixdevice);
    }
}

void MDWSwitch::setDisabled()
{
    setDisabled( true );
}

void MDWSwitch::setDisabled( bool value ) {
    if ( m_disabled!=value)
    {
	value ? hide() : show();
	m_disabled = value;
    }
}

/**
 * An event filter for the various QWidgets. We watch for Mouse press Events, so
 * that we can popup the context menu.
 */
bool MDWSwitch::eventFilter( QObject* obj, QEvent* e )
{
    if (e->type() == QEvent::MouseButtonPress) {
	QMouseEvent *qme = static_cast<QMouseEvent*>(e);
	if (qme->button() == Qt::RightButton) {
	    showContextMenu();
	    return true;
	}
    }
    return QWidget::eventFilter(obj,e);
}

#include "mdwswitch.moc"
