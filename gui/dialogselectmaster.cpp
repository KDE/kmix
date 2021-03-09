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

#include "gui/dialogselectmaster.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QListWidget>
#include <QComboBox>
#include <QToolTip>
#include <QStandardItemModel>

#include <klocalizedstring.h>

#include "core/ControlManager.h"
#include "core/mixer.h"
#include "gui/kmixtoolbox.h"


// TOD: Should this be incorporated into the "Configure KMix" dialogue?

DialogSelectMaster::DialogSelectMaster(const Mixer *mixer, QWidget *parent)
    : DialogBase(parent)
{
    setWindowTitle(i18n("Select Master Channel"));
    if (Mixer::mixers().count()>0) setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    else setButtons(QDialogButtonBox::Cancel);

   m_channelSelector = nullptr;
   createWidgets(mixer);				// for the specified 'mixer'
}


/**
 * Create basic widgets of the dialogue.
 */
void DialogSelectMaster::createWidgets(const Mixer *mixer)
{
    QWidget *mainFrame = new QWidget(this);
    setMainWidget(mainFrame);
    QVBoxLayout *layout = new QVBoxLayout(mainFrame);

    const QList<Mixer *> &mixers = Mixer::mixers();	// list of all mixers present

    if (mixers.count()>1)
    {
        int mixerIndex = 0;				// index of selected mixer

        // More than one Mixer => show Combo-Box to select Mixer
        // Mixer widget line
        QHBoxLayout *mixerNameLayout = new QHBoxLayout();
        layout->addLayout( mixerNameLayout );
        mixerNameLayout->setContentsMargins(0, 0, 0, 0);
    
        QLabel *qlbl = new QLabel( i18n("Current mixer:"), mainFrame );
        mixerNameLayout->addWidget(qlbl);
        qlbl->setFixedHeight(qlbl->sizeHint().height());
    
        m_cMixer = new QComboBox(mainFrame);
        m_cMixer->setObjectName( QLatin1String( "mixerCombo" ) );
        m_cMixer->setFixedHeight(m_cMixer->sizeHint().height());
        connect(m_cMixer, QOverload<int>::of(&QComboBox::activated), this, &DialogSelectMaster::createPageByID);

        for (int i = 0; i<mixers.count(); ++i)
        {
            const Mixer *m = mixers[i];
            m_cMixer->addItem(QIcon::fromTheme(m->iconName()), m->readableName(), m->id());
            if (m->id()==mixer->id()) mixerIndex = i;

            const MixSet &mixset = m->getMixSet();
            bool hasValidVolume = false;
            for (int j = 0; j<mixset.count(); ++j)
            {
                const shared_ptr<MixDevice> md = mixset[j];
                if (md->playbackVolume().hasVolume())
                {
                    hasValidVolume = true;
                    break;
                }
            }

            if (!hasValidVolume)
            {
                qCDebug(KMIX_LOG) << "mixer" << m->readableName() << "has no valid volume";
                QStandardItemModel *model = qobject_cast<QStandardItemModel*>(m_cMixer->model());
                QStandardItem *item = model->item(i);
                item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));

                // Need to set the item colour role so that it is shown in the disabled
                // colour.  See https://stackoverflow.com/questions/11439773/
                item->setData(m_cMixer->palette().brush(QPalette::Disabled, QPalette::Text), Qt::ForegroundRole);
            }
         }

        // Select the specified 'mixer' as the current item in the combo box.
        // If it was not found by the loop above, then select the first item.
        m_cMixer->setCurrentIndex(mixerIndex);
        
        mixerNameLayout->addWidget(m_cMixer, 1);
        layout->addSpacing(DialogBase::verticalSpacing());
    }

    if (mixers.count()>0)
    {
        QLabel *qlbl = new QLabel(xi18nc("@info", "Select the <link url=\"i\">master volume</link> channel:"), mainFrame);
        // Not a good idea to set Qt::LinksAccessibleByKeyboard, if the link is
        // clicked on or gets focus then the Return key will not activate the OK
        // button as expected.
        //qlbl->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);
        layout->addWidget(qlbl);
        connect(qlbl, &QLabel::linkActivated, this, [this]() {
            QToolTip::showText(QCursor::pos(), xi18nc("@info:tooltip",
"<para>Here you can select the master sound device (if there is more than one) and its master channel.</para>"
"<para>The master channel is the one that is affected by the system tray volume control, "
"the volume up/down and mute global shortcut keys, "
"and the <interface>Mute</interface> action in the system tray popup menu.</para>"));
        });

	m_channelSelector = new QListWidget(mainFrame);
#ifndef QT_NO_ACCESSIBILITY
        m_channelSelector->setAccessibleName(i18n("Select Master Channel"));
#endif
	m_channelSelector->setSelectionMode(QAbstractItemView::SingleSelection);
	m_channelSelector->setDragEnabled(false);
	m_channelSelector->setAlternatingRowColors(true);
        connect(m_channelSelector, &QListWidget::itemSelectionChanged, this, &DialogSelectMaster::slotUpdateButtons);
	layout->addWidget(m_channelSelector);

        connect(this, &QDialog::accepted, this, &DialogSelectMaster::apply);
        createPage(mixer);
    }
    else
    {
	QWidget *noMixersWarning = KMixToolBox::noDevicesWarningWidget(mainFrame);
        layout->addWidget(noMixersWarning);
        layout->addStretch(1);
    }
}


/**
 * Create the channel list for the specified mixer
 *
 * @p mixerId the ID (index) of the mixer for which the list should be created.
 */
void DialogSelectMaster::createPageByID(int mixerId)
{
    // TODO: would Mixer::mixers().value(mixerId) work just as well?
    // TODO: mixerId should really be named mixerIndex!
    const QString mixer_id = m_cMixer->itemData(mixerId).toString();
    const Mixer *mixer = Mixer::findMixer(mixer_id);
    if (mixer!=nullptr) createPage(mixer);
}


/**
 * Create the channel list for the specified mixer
 *
 * @p mixer the mixer for which the list should be created
 */
void DialogSelectMaster::createPage(const Mixer *mixer)
{
	// Clear the existing list widget.
	// This should automatically remove and delete all contained items.
	m_channelSelector->clear();

        const MixSet &mixset = mixer->getMixSet();
	const MasterControl mc = mixer->getGlobalMasterPreferred(false);
	QString masterKey = mc.getControl();
	if (!masterKey.isEmpty() && !mixset.get(masterKey))
	{
		const shared_ptr<MixDevice> master = mixer->getLocalMasterMD();
		if (master.get()!=nullptr) masterKey = master->id();
	}

	int msetCount = 0;
	for (int i = 0; i<mixset.count(); ++i)
        {
            const shared_ptr<MixDevice> md = mixset[i];
            if (md->playbackVolume().hasVolume()) ++msetCount;
        }

	if (msetCount>0 && !mixer->isDynamic())
	{
            QString mdName = i18n("Automatic (%1 recommendation)", mixer->getDriverName());
            auto *item = new QListWidgetItem(QIcon::fromTheme("audio-volume-high"), mdName, m_channelSelector);
            item->setData(Qt::UserRole, QString());	// ID here: see apply(), empty String => Automatic
            if (masterKey.isEmpty()) m_channelSelector->setCurrentItem(item);
	}

	// Populate the list view with the MixDevice's having a playback volume
        // (excludes pure capture controls and pure enum's)
	for (int i = 0; i<mixset.count(); ++i)
        {
            const shared_ptr<MixDevice> md = mixset[i];
            if (md->playbackVolume().hasVolume())
            {
                const QString mdName = md->readableName();
                auto *item = new QListWidgetItem(QIcon::fromTheme(md->iconName()), mdName, m_channelSelector);
                item->setData(Qt::UserRole, md->id());  // ID here: see apply()
                if (md->id() == masterKey)
                {					// select the current master
                    m_channelSelector->setCurrentItem(item);
                }
            }
        }

        slotUpdateButtons();
}


void DialogSelectMaster::slotUpdateButtons()
{
    const QList<QListWidgetItem *> items = m_channelSelector->selectedItems();
    setButtonEnabled(QDialogButtonBox::Ok, items.count()>0);
}


void DialogSelectMaster::apply()
{
    const QList<Mixer *> &mixers = Mixer::mixers();	// list of all mixers present
    Mixer *mixer = nullptr;				// selected mixer found

    if (mixers.count()==1)
    {
        // only one mixer => no combo box => take first (and only) entry
        mixer = mixers.first();
    }
    else if (mixers.count()>1)
    {
        // use the mixer that is currently selected in the combo box
        const int idx = m_cMixer->currentIndex();
        if (idx!=-1) mixer = mixers.at(idx);
    }

    if (mixer==nullptr)					// no mixers present,
    {							// user must have unplugged everything
        qCWarning(KMIX_LOG) << "no selected mixer";
        return;
    }

    const QList<QListWidgetItem *> items = m_channelSelector->selectedItems();
    if (items.count()==1)
    {
	const QListWidgetItem *item = items.first();
    	QString control_id = item->data(Qt::UserRole).toString();
        mixer->setLocalMasterMD(control_id);
        Mixer::setGlobalMaster(mixer->id(), control_id, true);
        ControlManager::instance().announce(mixer->id(), ControlManager::MasterChanged, QString("Select Master Dialog"));
    }
    else qCWarning(KMIX_LOG) << "no selected channel";
}
