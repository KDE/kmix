/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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

#include "gui/dialogchoosebackends.h"

#include <qbuttongroup.h>
#include <QCheckBox>
#include <QLabel>
#include <QSet>
#include <qscrollarea.h>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kcombobox.h>
#include <klocalizedstring.h>

#include "core/ControlManager.h"
#include "core/GlobalConfig.h"
#include "core/mixdevice.h"
#include "core/mixer.h"

/**
 * Creates a dialog to choose mixers from. All currently known mixers will be shown, and the given mixerID's
 * will be preselected.
 *
 * @param mixerIds A set of preselected mixer ID's
 * @param noButtons is a migration option. When DialogChooseBackends has been integrated as a Tab, it will be removed.
 */
DialogChooseBackends::DialogChooseBackends(QWidget* parent, const QSet<QString>& mixerIds)
  :  QWidget(parent), modified(false)
{
//    setCaption( i18n( "Select Mixers" ) );
//   	setButtons( None );

   _layout = 0;
   m_vboxForScrollView = 0;
   m_scrollableChannelSelector = 0;
   m_buttonGroupForScrollView = 0;
   createWidgets(mixerIds);

}

DialogChooseBackends::~DialogChooseBackends()
{
   delete _layout;
   delete m_vboxForScrollView;
}

/**
 * Create basic widgets of the Dialog.
 */
void DialogChooseBackends::createWidgets(const QSet<QString>& mixerIds)
{
	m_mainFrame = this;
//    m_mainFrame = new QFrame( this );
//    setMainWidget( m_mainFrame );
    _layout = new QVBoxLayout(m_mainFrame);
    _layout->setMargin(0);

    if ( !Mixer::mixers().isEmpty() )
    {
        QLabel *qlbl = new QLabel( i18n("Select the Mixers to display in the sound menu"), m_mainFrame );
        _layout->addWidget(qlbl);
    
        createPage(mixerIds);
    }
    else
    {
        QLabel *qlbl = new QLabel( i18n("No sound card is installed or currently plugged in."), m_mainFrame );
        _layout->addWidget(qlbl);
    }
}


/**
 * Create RadioButton's for the Mixer with number 'mixerId'.
 * @par mixerId The Mixer, for which the RadioButton's should be created.
 */
void DialogChooseBackends::createPage(const QSet<QString>& mixerIds)
{
	m_buttonGroupForScrollView = new QButtonGroup(this); // invisible QButtonGroup
	m_scrollableChannelSelector = new QScrollArea(m_mainFrame);

#ifndef QT_NO_ACCESSIBILITY
	m_scrollableChannelSelector->setAccessibleName(i18n("Select Mixers"));
#endif

	_layout->addWidget(m_scrollableChannelSelector);

	m_vboxForScrollView = new KVBox();

	bool hasMixerFilter = !mixerIds.isEmpty();
	qCDebug(KMIX_LOG) << "MixerIds=" << mixerIds;
	foreach ( Mixer* mixer, Mixer::mixers())
	{
		QCheckBox* qrb = new QCheckBox(mixer->readableName(true), m_vboxForScrollView);
		qrb->setObjectName(mixer->id());// The object name is used as ID here: see getChosenBackends()
		connect(qrb, SIGNAL(stateChanged(int)), SLOT(backendsModifiedSlot()));
		checkboxes.append(qrb);
		bool mixerShouldBeShown = !hasMixerFilter || mixerIds.contains(mixer->id());
		qrb->setChecked(mixerShouldBeShown);
	}

	m_scrollableChannelSelector->setWidget(m_vboxForScrollView);
	m_vboxForScrollView->show();  // show() is necessary starting with the second call to createPage()
}

QSet<QString> DialogChooseBackends::getChosenBackends()
{
	QSet<QString> newMixerList;
    foreach ( QCheckBox* qcb, checkboxes)
    {
    	if (qcb->isChecked())
    	{
    		newMixerList.insert(qcb->objectName());
    		qCDebug(KMIX_LOG) << "apply found " << qcb->objectName();
    	}
    }
    qCDebug(KMIX_LOG) << "New list is " << newMixerList;
    return newMixerList;
}

/**
 * Returns whether there were any modifications (activation/deactivation) and resets the flag.
 * @return
 */
bool DialogChooseBackends::getAndResetModifyFlag()
{
	bool modifiedOld = modified;
	modified = false;
	return modifiedOld;
}

bool DialogChooseBackends::getModifyFlag()
{
	return modified;
}

void DialogChooseBackends::backendsModifiedSlot()
{
	modified = true;
	emit backendsModified();
}


