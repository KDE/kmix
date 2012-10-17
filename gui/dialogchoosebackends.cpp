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
#include <kdebug.h>
#include <klocale.h>

#include "core/ControlManager.h"
#include "core/GlobalConfig.h"
#include "core/mixdevice.h"
#include "core/mixer.h"

DialogChooseBackends::DialogChooseBackends(QSet<QString>& mixerIds)
  : KDialog(  0 )
{
    setCaption( i18n( "Select Mixers" ) );
    if ( Mixer::mixers().count() > 0 )
        setButtons( Ok|Cancel );
    else {
        setButtons( Cancel );
    }
    setDefaultButton( Ok );
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
void DialogChooseBackends::createWidgets(QSet<QString>& mixerIds)
{
    m_mainFrame = new QFrame( this );
    setMainWidget( m_mainFrame );
    _layout = new QVBoxLayout(m_mainFrame);
    _layout->setMargin(0);

    if ( !Mixer::mixers().isEmpty() )
    {
        QLabel *qlbl = new QLabel( i18n("Select the Mixers to display in the sound menu"), m_mainFrame );
        _layout->addWidget(qlbl);
    
        createPage(mixerIds);
        connect( this, SIGNAL(okClicked())   , this, SLOT(apply()) );
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
void DialogChooseBackends::createPage(QSet<QString>& mixerIds)
{
	m_buttonGroupForScrollView = new QButtonGroup(this); // invisible QButtonGroup
	m_scrollableChannelSelector = new QScrollArea(m_mainFrame);

#ifndef QT_NO_ACCESSIBILITY
	m_scrollableChannelSelector->setAccessibleName(i18n("Select Mixers"));
#endif

	_layout->addWidget(m_scrollableChannelSelector);

	m_vboxForScrollView = new KVBox();

	kDebug() << "MixerIds=" << mixerIds;
	foreach ( Mixer* mixer, Mixer::mixers())
	{
		QString mdName = mixer->readableName();

		mdName.replace('&', "&&"); // Quoting the '&' needed, to prevent QCheckBox creating an accelerator
		QCheckBox* qrb = new QCheckBox( mdName, m_vboxForScrollView);
		qrb->setObjectName(mixer->id());// The object name is used as ID here: see apply()
//		m_buttonGroupForScrollView->addButton(qrb); // TODO remove m_buttonGroupForScrollView
		checkboxes.append(qrb);
		qrb->setChecked( mixerIds.contains(mixer->id()) );// preselect the current master
	}

	m_scrollableChannelSelector->setWidget(m_vboxForScrollView);
	m_vboxForScrollView->show();  // show() is necessary starting with the second call to createPage()
}


void DialogChooseBackends::apply()
{
	QSet<QString> newMixerList;
    foreach ( QCheckBox* qcb, checkboxes)
    {
    	if (qcb->isChecked())
    	{
    		newMixerList.insert(qcb->objectName());
    		kDebug() << "apply found " << qcb->objectName();
    	}
    }

    // Announcing MasterChanged, as the sound menu (aka ViewDockAreaPopup) primarily shows master volume(s).
    // In any case, ViewDockAreaPopup treats MasterChanged and ControlList the same, so it is better to announce
    // the "smaller" change.
    kDebug() << "New list is " << newMixerList;
    GlobalConfig::instance().setMixersForSoundmenu(newMixerList);
	ControlManager::instance().announce(QString(), ControlChangeType::MasterChanged, QString("Select Backends Dialog"));
}

#include "dialogchoosebackends.moc"

