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

#include <kiconloader.h>
#include <klocalizedstring.h>

#include "core/ControlManager.h"
#include "core/mixdevice.h"
#include "core/mixer.h"

DialogSelectMaster::DialogSelectMaster( Mixer *mixer, QWidget *parent )
  : DialogBase( parent )
{
    setWindowTitle(i18n("Select Master Channel"));
    if ( Mixer::mixers().count() > 0 )
        setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    else {
        setButtons(QDialogButtonBox::Cancel);
    }

   m_channelSelector = 0;
   createWidgets(mixer);  // Open with Mixer Hardware #0

}

/**
 * Create basic widgets of the Dialog.
 */
void DialogSelectMaster::createWidgets(Mixer *ptr_mixer)
{
    QWidget *mainFrame = new QWidget(this);
    setMainWidget(mainFrame);
    QVBoxLayout *layout = new QVBoxLayout(mainFrame);

    if ( Mixer::mixers().count() > 1 ) {
        // More than one Mixer => show Combo-Box to select Mixer
        // Mixer widget line
        QHBoxLayout* mixerNameLayout = new QHBoxLayout();
        layout->addLayout( mixerNameLayout );
        mixerNameLayout->setMargin(0);
        mixerNameLayout->setSpacing(DialogBase::horizontalSpacing());
    
        QLabel *qlbl = new QLabel( i18n("Current mixer:"), mainFrame );
        mixerNameLayout->addWidget(qlbl);
        qlbl->setFixedHeight(qlbl->sizeHint().height());
    
        m_cMixer = new QComboBox(mainFrame);
        m_cMixer->setObjectName( QLatin1String( "mixerCombo" ) );
        m_cMixer->setFixedHeight(m_cMixer->sizeHint().height());
        connect( m_cMixer, SIGNAL(activated(int)), this, SLOT(createPageByID(int)) );

        for( int i =0; i<Mixer::mixers().count(); i++ )
        {
            const Mixer *mixer = (Mixer::mixers())[i];
            const shared_ptr<MixDevice> md = mixer->getLocalMasterMD();
            const QString iconName = (md!=nullptr) ? md->iconName() : "media-playback-start";
            m_cMixer->addItem(QIcon::fromTheme(iconName), mixer->readableName(), mixer->id() );
         } // end for all_Mixers
        // Make the current Mixer the current item in the ComboBox
        int findIndex = m_cMixer->findData( ptr_mixer->id() );
        if ( findIndex != -1 ) m_cMixer->setCurrentIndex( findIndex );
        
        m_cMixer->setToolTip( i18n("Current mixer" ) );
        mixerNameLayout->addWidget(m_cMixer, 1);
        layout->addSpacing(DialogBase::verticalSpacing());

    } // end if (more_than_1_Mixer)

    if ( Mixer::mixers().count() > 0 ) {
        QLabel *qlbl = new QLabel( i18n("Select the channel representing the master volume:"), mainFrame );
        layout->addWidget(qlbl);
    
        createPage(ptr_mixer);
        connect(this, SIGNAL(accepted()), this, SLOT(apply()));
    }
    else {
        QLabel *qlbl = new QLabel( i18n("No sound card is installed or currently plugged in."), mainFrame );
        layout->addWidget(qlbl);
    }
}

/**
 * Create RadioButton's for the Mixer with number 'mixerId'.
 * @par mixerId The Mixer, for which the RadioButton's should be created.
 */
void DialogSelectMaster::createPageByID(int mixerId)
{
    QString mixer_id = m_cMixer->itemData(mixerId).toString();
    Mixer * mixer = Mixer::findMixer(mixer_id);

    if ( mixer != NULL )
        createPage(mixer);
}

/**
 * Create RadioButton's for the Mixer with number 'mixerId'.
 * @par mixerId The Mixer, for which the RadioButton's should be created.
 */
void DialogSelectMaster::createPage(Mixer* mixer)
{

    /** --- Reset page -----------------------------------------------
     * In case the user selected a new Mixer via m_cMixer, we need
     * to remove the stuff created on the last call.
     */
	// delete the list widget.
	// This should automatically remove all contained items.
	delete m_channelSelector;

    
    /** Reset page end -------------------------------------------------- */
    
        QWidget *mainFrame = mainWidget();
        QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(mainFrame->layout());
        Q_ASSERT(layout!=nullptr);

	m_channelSelector = new QListWidget(mainFrame);
#ifndef QT_NO_ACCESSIBILITY
        m_channelSelector->setAccessibleName( i18n("Select Master Channel") );
#endif
	m_channelSelector->setSelectionMode(QAbstractItemView::SingleSelection);
	m_channelSelector->setDragEnabled(false);
	m_channelSelector->setAlternatingRowColors(true);
	layout->addWidget(m_channelSelector);

//    shared_ptr<MixDevice> master = mixer->getLocalMasterMD();
//    QString masterKey = ( master.get() != 0 ) ? master->id() : "----noMaster---"; // Use non-matching name as default

        const MixSet& mixset = mixer->getMixSet();
	MixSet& mset = const_cast<MixSet&>(mixset);

	MasterControl mc = mixer->getGlobalMasterPreferred(false);
	QString masterKey = mc.getControl();
	if (!masterKey.isEmpty() && !mset.get(masterKey))
	{
		shared_ptr<MixDevice> master = mixer->getLocalMasterMD();
		if (master.get() != 0)
			masterKey = master->id();
	}

	int msetCount = 0;
	for (int i = 0; i < mset.count(); ++i)
    {
    	shared_ptr<MixDevice> md = mset[i];
        if ( md->playbackVolume().hasVolume() )
        	++msetCount;
    }

	if (msetCount > 0 && !mixer->isDynamic())
	{
            QString mdName = i18n("Automatic (%1 recommendation)", mixer->getDriverName());
		QPixmap icon = KIconLoader::global()->loadScaledIcon("audio-volume-high", KIconLoader::Small, devicePixelRatioF(), IconSize(KIconLoader::Small));
        QListWidgetItem *item = new QListWidgetItem(icon, mdName, m_channelSelector);
        item->setData(Qt::UserRole, QString());  // ID here: see apply(), empty String => Automatic
		if (masterKey.isEmpty())
			m_channelSelector->setCurrentItem(item);
	}

	// Populate ListView with the MixDevice's having a playbakc volume (excludes pure capture controls and pure enum's)
	for (int i = 0; i < mset.count(); ++i)
    {
    	shared_ptr<MixDevice> md = mset[i];
        if ( md->playbackVolume().hasVolume() )
        {
            QString mdName = md->readableName();
			QPixmap icon = KIconLoader::global()->loadScaledIcon(md->iconName(), KIconLoader::Small, devicePixelRatioF(), IconSize(KIconLoader::Small));
            QListWidgetItem *item = new QListWidgetItem(icon, mdName, m_channelSelector);
            item->setData(Qt::UserRole, md->id());  // ID here: see apply()
            if ( md->id() == masterKey )
            {          // select the current master
                m_channelSelector->setCurrentItem(item);
            }
        }
    }

    setButtonEnabled(QDialogButtonBox::Ok, m_channelSelector->count()>0);
}


void DialogSelectMaster::apply()
{
    Mixer *mixer = 0;
    if ( Mixer::mixers().count() == 1 ) {
        // only one mxier => no combo box => take first entry
        mixer = (Mixer::mixers())[0];
    }
    else if ( Mixer::mixers().count() > 1 ) {
        // find mixer that is currently active in the ComboBox
        int idx = m_cMixer->currentIndex();
        QString mixer_id = m_cMixer->itemData(idx).toString();
        mixer = Mixer::findMixer(mixer_id);
    }

    if ( mixer == 0 )
    	 return; // User must have unplugged everything
    QList<QListWidgetItem *> items = m_channelSelector->selectedItems();
    if (items.count()==1)
    {
    	QListWidgetItem *item = items.first();
    	QString control_id = item->data(Qt::UserRole).toString();
		mixer->setLocalMasterMD( control_id );
		Mixer::setGlobalMaster(mixer->id(), control_id, true);
		ControlManager::instance().announce(mixer->id(), ControlManager::MasterChanged, QString("Select Master Dialog"));
   }
}


