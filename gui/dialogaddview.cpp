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

#include "gui/dialogaddview.h"

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>

#include <klocalizedstring.h>

#include "core/mixdevice.h"
#include "core/mixer.h"


static QStringList viewNames;
static QStringList viewIds;


DialogAddView::DialogAddView(QWidget *parent, Mixer *mixer)
    : DialogBase( parent )
{
	// TODO 000 Adding View for MPRIS2 is broken. We need at least a dummy XML GUI Profile. Also the
	//      fixed list below is plain wrong. Actually we should get the Profile list from either the XML files or
	//      from the backend. The latter is probably easier for now.
    if ( viewNames.isEmpty() )
    {
        // initialize static list. Later this list could be generated from the actually installed profiles
        viewNames.append(i18n("All controls"));
        viewNames.append(i18n("Only playback controls"));
        viewNames.append(i18n("Only capture controls"));

        viewIds.append("default");
        viewIds.append("playback");
        viewIds.append("capture");
    }

    setWindowTitle( i18n( "Add View" ) );
    if ( Mixer::mixers().count() > 0 )
        setButtons( QDialogButtonBox::Ok|QDialogButtonBox::Cancel );
    else {
        setButtons( QDialogButtonBox::Cancel );
    }

   m_listForChannelSelector = nullptr;
   createWidgets(mixer);  // Open with Mixer Hardware #0

}

/**
 * Create basic widgets of the Dialog.
 */
void DialogAddView::createWidgets(Mixer *ptr_mixer)
{
    QWidget *mainFrame = new QWidget(this);
    setMainWidget(mainFrame);
    QVBoxLayout *layout = new QVBoxLayout(mainFrame);

    if ( Mixer::mixers().count() > 1 ) {
        // More than one Mixer => show Combo-Box to select Mixer
        // Mixer widget line
        QHBoxLayout* mixerNameLayout = new QHBoxLayout();
        layout->addLayout(mixerNameLayout);
        mixerNameLayout->setSpacing(DialogBase::horizontalSpacing());
    
        QLabel *qlbl = new QLabel( i18n("Select mixer:"), mainFrame );
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
            m_cMixer->addItem(QIcon::fromTheme(iconName), mixer->readableName() );
         } // end for all_Mixers
        // Make the current Mixer the current item in the ComboBox
        int findIndex = m_cMixer->findText( ptr_mixer->readableName() );
        if ( findIndex != -1 ) m_cMixer->setCurrentIndex( findIndex );
        
    
        m_cMixer->setToolTip( i18n("Current mixer" ) );
        mixerNameLayout->addWidget(m_cMixer);
    
    } // end if (more_than_1_Mixer)

    
    if ( Mixer::mixers().count() > 0 ) {
        QLabel *qlbl = new QLabel( i18n("Select the design for the new view:"), mainFrame );
        layout->addWidget(qlbl);
    
        createPage(Mixer::mixers()[0]);
        connect( this, SIGNAL(accepted())   , this, SLOT(apply()) );
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
void DialogAddView::createPageByID(int mixerId)
{
  //qCDebug(KMIX_LOG) << "DialogAddView::createPage()";
    QString selectedMixerName = m_cMixer->itemText(mixerId);
    for( int i =0; i<Mixer::mixers().count(); i++ )
    {
        Mixer *mixer = (Mixer::mixers())[i];
        if ( mixer->readableName() == selectedMixerName ) {
            createPage(mixer);
            break;
        }
    } // for
}

/**
 * Create RadioButton's for the Mixer with number 'mixerId'.
 * @par mixerId The Mixer, for which the RadioButton's should be created.
 */
void DialogAddView::createPage(Mixer *mixer)
{
    /** --- Reset page -----------------------------------------------
     * In case the user selected a new Mixer via m_cMixer, we need
     * to remove the stuff created on the last call.
     */
    delete m_listForChannelSelector;
    setButtonEnabled(QDialogButtonBox::Ok, false);

        /** Reset page end -------------------------------------------------- */
    
    QWidget *mainFrame = mainWidget();
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(mainFrame->layout());
    Q_ASSERT(layout!=nullptr);

    m_listForChannelSelector = new QListWidget(mainFrame);
    m_listForChannelSelector->setUniformItemSizes(true);
    m_listForChannelSelector->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_listForChannelSelector, SIGNAL(itemSelectionChanged()), this, SLOT(profileSelectionChanged()));
    layout->addWidget(m_listForChannelSelector);

    for (int i = 0; i<viewNames.size(); ++i)
    {
    	QString viewId = viewIds.at(i);
    	if (viewId != "default" && mixer->isDynamic())
    	{
    		// TODO: The mixer's backend MUST be inspected to find out the supported profiles.
    		//       Hardcoding it here is only a quick workaround.
    		continue;
		}
		
        // Create an item for each view type
        QString name = viewNames.at(i);
        QListWidgetItem *item = new QListWidgetItem(m_listForChannelSelector);
        item->setText(name);
        item->setData(Qt::UserRole, viewIds.at(i));  // mixer ID as data
    }

    // If there is only one option available to select, then preselect it.
    if (m_listForChannelSelector->count()==1) m_listForChannelSelector->setCurrentRow(0);
}


void DialogAddView::profileSelectionChanged()
{
    QList<QListWidgetItem *> items = m_listForChannelSelector->selectedItems();
    setButtonEnabled(QDialogButtonBox::Ok, !items.isEmpty());
}

void DialogAddView::apply()
{
    Mixer *mixer = nullptr;
    if ( Mixer::mixers().count() == 1 ) {
        // only one mixer => no combo box => take first entry
        mixer = (Mixer::mixers())[0];
    }
    else if ( Mixer::mixers().count() > 1 ) {
        // find mixer that is currently active in the ComboBox
        QString selectedMixerName = m_cMixer->itemText(m_cMixer->currentIndex());
        
        for( int i =0; i<Mixer::mixers().count(); i++ )
        {
            mixer = (Mixer::mixers())[i];
            if ( mixer->readableName() == selectedMixerName ) {
                mixer = (Mixer::mixers())[i];
                break;
            }
        } // for
    }
    Q_ASSERT(mixer!=nullptr);

    QList<QListWidgetItem *> items = m_listForChannelSelector->selectedItems();
    if (items.isEmpty()) return;			// nothing selected
    QListWidgetItem *item = items.first();
    QString viewName = item->data(Qt::UserRole).toString();

    qCDebug(KMIX_LOG) << "We should now create a new view " << viewName << " for mixer " << mixer->id();
    resultMixerId = mixer->id();
    resultViewName = viewName;
}
