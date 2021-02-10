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

#include "core/mixer.h"
#include "gui/kmixtoolbox.h"

/**
 * Creates a dialog to choose mixers from. All currently known mixers will be shown, and the given mixerID's
 * will be preselected.
 *
 * @param mixerIds A set of preselected mixer ID's
 */
DialogChooseBackends::DialogChooseBackends(QWidget *parent, const QSet<QString> &mixerIds)
  :  QWidget(parent), modified(false)
{
   createWidgets(mixerIds);
}

/**
 * Create basic widgets of the dialogue.
 */
void DialogChooseBackends::createWidgets(const QSet<QString> &mixerIds)
{
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *topLabel = new QLabel(i18n("Mixers to show in the popup volume control:"), this);
    vLayout->addWidget(topLabel);

    createPage(mixerIds);
    vLayout->addWidget(m_mixerList, 1);
    topLabel->setBuddy(m_mixerList);

    if (Mixer::mixers().isEmpty())
    {
	QWidget *noMixersWarning = KMixToolBox::noDevicesWarningWidget(this);
        vLayout->addWidget(noMixersWarning);
	m_mixerList->setEnabled(false);
    }
}


/**
 * Create selection list for the specified mixers.
 */
void DialogChooseBackends::createPage(const QSet<QString> &mixerIds)
{
	m_mixerList = new QListWidget(this);
	m_mixerList->setUniformItemSizes(true);
	m_mixerList->setAlternatingRowColors(true);
	m_mixerList->setSelectionMode(QAbstractItemView::NoSelection);
	m_mixerList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#ifndef QT_NO_ACCESSIBILITY
	m_mixerList->setAccessibleName(i18n("Select Mixers"));
#endif
	bool hasMixerFilter = !mixerIds.isEmpty();
	qCDebug(KMIX_LOG) << "MixerIds=" << mixerIds;
	for (const Mixer *mixer : qAsConst(Mixer::mixers()))
	{
            // TODO: No point in showing mixers which do not have any volume controls.
            // See checks done in ViewDockAreaPopup::initLayout()

            QListWidgetItem *item = new QListWidgetItem(m_mixerList);
            item->setText(mixer->readableName(true));
            item->setSizeHint(QSize(1, 16));
            item->setIcon(QIcon::fromTheme(mixer->iconName()));
            item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable|Qt::ItemNeverHasChildren);
            item->setData(Qt::UserRole, mixer->id());

            const bool mixerShouldBeShown = !hasMixerFilter || mixerIds.contains(mixer->id());
            item->setCheckState(mixerShouldBeShown ? Qt::Checked : Qt::Unchecked);
	}

        connect(m_mixerList, &QListWidget::itemChanged, this, &DialogChooseBackends::backendsModifiedSlot);
        connect(m_mixerList, &QListWidget::itemActivated, this, &DialogChooseBackends::itemActivatedSlot);
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

bool DialogChooseBackends::getModifyFlag() const
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
