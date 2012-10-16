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

#include <qbuttongroup.h>
#include <QLabel>
#include <qradiobutton.h>
#include <qscrollarea.h>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kcombobox.h>
#include <kdebug.h>
#include <klocale.h>

#include "core/ControlManager.h"
#include "core/mixdevice.h"
#include "core/mixer.h"

DialogSelectMaster::DialogSelectMaster( Mixer *mixer  )
  : KDialog(  0 )
{
    setCaption( i18n( "Select Master Channel" ) );
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
   createWidgets(mixer);  // Open with Mixer Hardware #0

}

DialogSelectMaster::~DialogSelectMaster()
{
   delete _layout;
   delete m_vboxForScrollView;
}

/**
 * Create basic widgets of the Dialog.
 */
void DialogSelectMaster::createWidgets(Mixer *ptr_mixer)
{
    m_mainFrame = new QFrame( this );
    setMainWidget( m_mainFrame );
    _layout = new QVBoxLayout(m_mainFrame);
    _layout->setMargin(0);

    if ( Mixer::mixers().count() > 1 ) {
        // More than one Mixer => show Combo-Box to select Mixer
        // Mixer widget line
        QHBoxLayout* mixerNameLayout = new QHBoxLayout();
        _layout->addItem( mixerNameLayout );
        mixerNameLayout->setSpacing(KDialog::spacingHint());
    
        QLabel *qlbl = new QLabel( i18n("Current mixer:"), m_mainFrame );
        mixerNameLayout->addWidget(qlbl);
        qlbl->setFixedHeight(qlbl->sizeHint().height());
    
        m_cMixer = new KComboBox( false, m_mainFrame);
        m_cMixer->setObjectName( QLatin1String( "mixerCombo" ) );
        m_cMixer->setFixedHeight(m_cMixer->sizeHint().height());
        connect( m_cMixer, SIGNAL(activated(int)), this, SLOT(createPageByID(int)) );

        for( int i =0; i<Mixer::mixers().count(); i++ )
        {
            Mixer *mixer = (Mixer::mixers())[i];
            m_cMixer->addItem( mixer->readableName(), mixer->id() );
         } // end for all_Mixers
        // Make the current Mixer the current item in the ComboBox
        int findIndex = m_cMixer->findData( ptr_mixer->id() );
        if ( findIndex != -1 ) m_cMixer->setCurrentIndex( findIndex );
        
    
        m_cMixer->setToolTip( i18n("Current mixer" ) );
        mixerNameLayout->addWidget(m_cMixer);
    
    } // end if (more_than_1_Mixer)

    
    if ( Mixer::mixers().count() > 0 ) {
        QLabel *qlbl = new QLabel( i18n("Select the channel representing the master volume:"), m_mainFrame );
        _layout->addWidget(qlbl);
    
        createPage(ptr_mixer);
        connect( this, SIGNAL(okClicked())   , this, SLOT(apply()) );
    }
    else {
        QLabel *qlbl = new QLabel( i18n("No sound card is installed or currently plugged in."), m_mainFrame );
        _layout->addWidget(qlbl);
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
    // delete the VBox. This should automatically remove all contained QRadioButton's.
    delete m_vboxForScrollView;
    delete m_scrollableChannelSelector;
    delete m_buttonGroupForScrollView;
    
    /** Reset page end -------------------------------------------------- */
    
    m_buttonGroupForScrollView = new QButtonGroup(this); // invisible QButtonGroup
    //m_buttonGroupForScrollView->hide();

    m_scrollableChannelSelector = new QScrollArea(m_mainFrame);

#ifndef QT_NO_ACCESSIBILITY
    m_scrollableChannelSelector->setAccessibleName( i18n("Select Master Channel") );
#endif

//    m_scrollableChannelSelector->viewport()->setBackgroundRole(QPalette::Background);
    _layout->addWidget(m_scrollableChannelSelector);

    m_vboxForScrollView = new KVBox(); //m_scrollableChannelSelector->viewport()

    QString masterKey = "----noMaster---";  // Use a non-matching name as default
    shared_ptr<MixDevice> master = mixer->getLocalMasterMD();
    if ( master.get() != 0 )
    	masterKey = master->id();

    const MixSet& mixset = mixer->getMixSet();
    MixSet& mset = const_cast<MixSet&>(mixset);
    for( int i=0; i< mset.count(); ++i )
    {
    	shared_ptr<MixDevice> md = mset[i];
        // Create a RadioButton for each MixDevice (excluding Enum's)
        if ( md->playbackVolume().hasVolume() )
        {
//            kDebug(67100) << "DialogSelectMaster::createPage() mset append qrb";
            QString mdName = md->readableName();
            mdName.replace('&', "&&"); // Quoting the '&' needed, to prevent QRadioButton creating an accelerator
            QRadioButton* qrb = new QRadioButton( mdName, m_vboxForScrollView);
            qrb->setObjectName(md->id());  // The object name is used as ID here: see apply()
            m_buttonGroupForScrollView->addButton(qrb);  //(qrb, md->num());
            //_qEnabledCB.append(qrb);
            //m_mixerPKs.push_back(md->id());
            if ( md->id() == masterKey ) {
                qrb->setChecked(true); // preselect the current master
            }
            else {
                qrb->setChecked(false);
            }
        }
    }

    m_scrollableChannelSelector->setWidget(m_vboxForScrollView);
    m_vboxForScrollView->show();  // show() is necessary starting with the second call to createPage()
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
   
    QAbstractButton* button =  m_buttonGroupForScrollView->checkedButton();
    if ( button != 0 )
    {
		QString control_id = button->objectName();
		mixer->setLocalMasterMD( control_id );
		Mixer::setGlobalMaster(mixer->id(), control_id, true);
		ControlManager::instance().announce(mixer->id(), ControlChangeType::MasterChanged, QString("Select Master Dialog"));
   }
}

#include "dialogselectmaster.moc"

