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

#include <qlabel.h>
#include <qset.h>
#include <QVBoxLayout>
#include <qlistwidget.h>

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
   createWidgets(mixerIds);

}

/**
 * Create basic widgets of the Dialog.
 */
void DialogChooseBackends::createWidgets(const QSet<QString>& mixerIds)
{
    _layout = new QVBoxLayout(this);
    _layout->setMargin(0);

    if ( !Mixer::mixers().isEmpty() )
    {
        m_listLabel = new QLabel( i18n("Mixers to show in the popup volume control:"), this);
        _layout->addWidget(m_listLabel);
        createPage(mixerIds);
    }
    else
    {
        m_listLabel = new QLabel( i18n("No sound card is installed or currently plugged in."), this);
        _layout->addWidget(m_listLabel);
    }
}


/**
 * Create RadioButton's for the Mixer with number 'mixerId'.
 * @par mixerId The Mixer, for which the RadioButton's should be created.
 */
void DialogChooseBackends::createPage(const QSet<QString>& mixerIds)
{
	m_mixerList = new QListWidget(this);
	m_mixerList->setUniformItemSizes(true);
	m_mixerList->setAlternatingRowColors(true);
	m_mixerList->setSelectionMode(QAbstractItemView::NoSelection);
	m_mixerList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#ifndef QT_NO_ACCESSIBILITY
	m_mixerList->setAccessibleName(i18n("Select Mixers"));
#endif
	m_listLabel->setBuddy(m_mixerList);

	_layout->addWidget(m_mixerList);

	bool hasMixerFilter = !mixerIds.isEmpty();
	qCDebug(KMIX_LOG) << "MixerIds=" << mixerIds;
	foreach ( Mixer* mixer, Mixer::mixers())
	{
            QListWidgetItem *item = new QListWidgetItem(m_mixerList);
            item->setText(mixer->readableName(true));
            item->setSizeHint(QSize(1, 16));
            item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable|Qt::ItemNeverHasChildren);
            const bool mixerShouldBeShown = !hasMixerFilter || mixerIds.contains(mixer->id());
            item->setCheckState(mixerShouldBeShown ? Qt::Checked : Qt::Unchecked);
            item->setData(Qt::UserRole, mixer->id());
	}

        connect(m_mixerList, SIGNAL(itemChanged(QListWidgetItem*)), SLOT(backendsModifiedSlot()));
        connect(m_mixerList, SIGNAL(itemActivated(QListWidgetItem*)), SLOT(itemActivatedSlot(QListWidgetItem*)));
}

QSet<QString> DialogChooseBackends::getChosenBackends()
{
	QSet<QString> newMixerList;
        for (int row = 0; row<m_mixerList->count(); ++row)
        {
            QListWidgetItem *item = m_mixerList->item(row);
            if (item->checkState()==Qt::Checked)
            {
		const QString mixer = item->data(Qt::UserRole).toString();
		newMixerList.insert(mixer);
		qCDebug(KMIX_LOG) << "apply found " << mixer;
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


void DialogChooseBackends::itemActivatedSlot(QListWidgetItem *item)
{
	item->setCheckState(item->checkState()==Qt::Checked ? Qt::Unchecked : Qt::Checked);
}
