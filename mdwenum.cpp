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
#include <qtooltip.h>

#include <klocale.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kaction.h>
#include <kpopupmenu.h>

#include <kglobalaccel.h>
#include <kkeydialog.h>

#include <kdebug.h>

#include "mdwenum.h"
#include "mixer.h"
#include "viewbase.h"

/**
 * Class that represents an Enum element (a select one-from-many selector)
 * The orientation (horizontal, vertical) is ignored
 */
MDWEnum::MDWEnum(Mixer *mixer, MixDevice* md,
                                 Qt::Orientation orientation,
                                 QWidget* parent, ViewBase* mw, const char* name) :
    MixDeviceWidget(mixer,md,false,orientation,parent,mw,name),
     _label(0), _enumCombo(0), _layout(0)
{
    // create actions (on _mdwActions, see MixDeviceWidget)

    // KStdAction::showMenubar() is in MixDeviceWidget now
    new KToggleAction( i18n("&Hide"), 0, this, SLOT(setDisabled()), _mdwActions, "hide" );
    new KAction( i18n("C&onfigure Shortcuts..."), 0, this, SLOT(defineKeys()), _mdwActions, "keys" );

    // create widgets
    createWidgets();

    /* !!! remove this for production version */
    m_keys->insert( "Next Value", i18n( "Next Value" ), QString::null,
		    KShortcut(), KShortcut(), this, SLOT( nextEnumId() ) );

    installEventFilter( this ); // filter for popup
}

MDWEnum::~MDWEnum()
{
}


void MDWEnum::createWidgets()
{
	if ( _orientation == Qt::Vertical ) {
		_layout = new QVBoxLayout( this );
		_layout->setAlignment(Qt::AlignHCenter);
	}
	else {
		_layout = new QHBoxLayout( this );
		_layout->setAlignment(Qt::AlignVCenter);
	}
	QToolTip::add( this, m_mixdevice->name() );
	
        //this->setStretchFactor( _layout, 0 );
        //QSizePolicy qsp( QSizePolicy::Ignored, QSizePolicy::Maximum);
        //_layout->setSizePolicy(qsp);
        //_layout->setSpacing(KDialog::spacingHint());
        _label = new QLabel( m_mixdevice->name(), this);
	_layout->addWidget(_label);
        _label->setFixedHeight(_label->sizeHint().height());
        _enumCombo = new KComboBox( FALSE, this, "mixerCombo" );
	// ------------ fill ComboBox start ------------
	int maxEnumId= m_mixdevice->enumValues().count();
	for (int i=0; i<maxEnumId; i++ ) {
	  _enumCombo->insertItem( *(m_mixdevice->enumValues().at(i)),i);
	}
	// ------------ fill ComboBox end --------------
	_layout->addWidget(_enumCombo);
        _enumCombo->setFixedHeight(_enumCombo->sizeHint().height());
        connect( _enumCombo, SIGNAL( activated( int ) ), this, SLOT( setEnumId( int ) ) );
        QToolTip::add( _enumCombo, m_mixdevice->name() );
	
	//_layout->addSpacing( 4 );
}

void MDWEnum::update()
{
  if ( m_mixdevice->isEnum() ) {
    //kdDebug(67100) << "MDWEnum::update() enumID=" << m_mixdevice->enumId() << endl;
    _enumCombo->setCurrentItem( m_mixdevice->enumId() );
  }
  else {
    // !!! print warning message
  }	
}

void MDWEnum::showContextMenu()
{
    if( m_mixerwidget == NULL )
	return;

    KPopupMenu *menu = m_mixerwidget->getPopup();

    QPoint pos = QCursor::pos();
    menu->popup( pos );
}

QSize MDWEnum::sizeHint() const {
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
void MDWEnum::nextEnumId() {
  if( m_mixdevice->isEnum() ) {
    int curEnum = enumId();
    if ( (unsigned int)curEnum < m_mixdevice->enumValues().count() ) {
      // next enum value
      setEnumId(curEnum+1);
    }
    else {
      // wrap around
      setEnumId(0);
    }
  } // isEnum
}

void MDWEnum::setEnumId(int value)
{
	if (  m_mixdevice->isEnum() ) {
		m_mixdevice->setEnumId( value );
		m_mixer->commitVolumeChange( m_mixdevice );
	}
}

int MDWEnum::enumId()
{
	if (  m_mixdevice->isEnum() ) {
		return m_mixdevice->enumId();
	}
	else {
		return 0;
	}
}

void MDWEnum::setDisabled()
{
    setDisabled( true );
}

void MDWEnum::setDisabled( bool value ) {
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
bool MDWEnum::eventFilter( QObject* obj, QEvent* e )
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

#include "mdwenum.moc"
