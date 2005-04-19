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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qbuttongroup.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qptrlist.h>
#include <qradiobutton.h>
#include <qscrollview.h>
#include <qtooltip.h>
#include <qvbox.h>

#include <kcombobox.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <klocale.h>

#include "dialogselectmaster.h"
#include "mixdevice.h"
#include "mixer.h"

DialogSelectMaster::DialogSelectMaster( QWidget* )
    : KDialogBase(  Plain, i18n( "Configure" ), Ok|Cancel, Ok )
{
   _layout = 0;
   m_vboxForScrollView = 0;
   createWidgets(0);  // Open with Mixer Hardware #0

}

DialogSelectMaster::~DialogSelectMaster()
{
   delete _layout;
   delete m_vboxForScrollView;
}

/**
 * Create basic widgets of the Dialog.
 */
void DialogSelectMaster::createWidgets(int mixerId)
{
    QFrame *m_mainFrame = plainPage();
    _layout = new QVBoxLayout(m_mainFrame,0,-1, "_layout" );

    if ( Mixer::mixers().count() > 1 ) {
      kdDebug(67100) << "DialogSelectMaster::createPage count()>1" << "\n";
      // More than one Mixer => show Combo-Box to select Mixer
      // Mixer widget line
      QHBoxLayout* mixerNameLayout = new QHBoxLayout( _layout );
      //widgetsLayout->setStretchFactor( mixerNameLayout, 0 );
      //QSizePolicy qsp( QSizePolicy::Ignored, QSizePolicy::Maximum);
      //mixerNameLayout->setSizePolicy(qsp);
      mixerNameLayout->setSpacing(KDialog::spacingHint());

      QLabel *qlbl = new QLabel( i18n("Current Mixer"), m_mainFrame );
      mixerNameLayout->addWidget(qlbl);
      qlbl->setFixedHeight(qlbl->sizeHint().height());

      m_cMixer = new KComboBox( FALSE, m_mainFrame, "mixerCombo" );
      m_cMixer->setFixedHeight(m_cMixer->sizeHint().height());
      connect( m_cMixer, SIGNAL( activated( int ) ), this, SLOT( createPage( int ) ) );
      for ( Mixer *mixer = Mixer::mixers().first(); mixer !=0; mixer = Mixer::mixers().next() ) {
	m_cMixer->insertItem( mixer->mixerName() );
      }

      QToolTip::add( m_cMixer, i18n("Current mixer" ) );
      mixerNameLayout->addWidget(m_cMixer);
      m_cMixer->setCurrentItem(mixerId);
    }

    QLabel *qlbl = new QLabel( i18n("Select Channel"), m_mainFrame );
    _layout->addWidget(qlbl);

    m_scrollableChannelSelector = new QScrollView(m_mainFrame, "scrollableChannelSelector");
    m_scrollableChannelSelector->viewport()->setBackgroundMode(Qt::PaletteBackground);
    _layout->add(m_scrollableChannelSelector);

    m_buttonGroupForScrollView = new QButtonGroup(this); // invisible QButtonGroup
    m_buttonGroupForScrollView->hide();

    createPage(mixerId);
    connect( this, SIGNAL(okClicked())   , this, SLOT(apply()) );
}


/**
 * Create RadioButton's for the Mixer with number 'mixerId'.
 * @par mixerId The Mixer, for which the RadioButton's should be created.
 */
void DialogSelectMaster::createPage(int mixerId)
{
    //kdDebug(67100) << "DialogSelectMaster::createPage()" << endl;
    Mixer *mixer = Mixer::mixers().at(mixerId);
    if ( mixer == 0 ) {
      kdError(67100) << "DialogSelectMaster::createPage(): Invalid Mixer (mixerID=" << mixerId << ")" << endl;
      return; // can not happen
    }
    
    /** --- Reset page -----------------------------------------------
     * In case the user selected a new Mixer via m_cMixer, we need
     * to remove the stuff created on the last call.
     */
    //m_buttonGroupForScrollView->clear();
    //_qEnabledCB.setAutoDelete(true);
    //_qEnabledCB.clear();
    // delete the VBox. This should automatically remove all contained QRadioButton's.
    delete m_vboxForScrollView;
    /** Reset page end -------------------------------------------------- */

    m_vboxForScrollView = new QVBox(m_scrollableChannelSelector->viewport());
    m_scrollableChannelSelector->addChild(m_vboxForScrollView);


    const MixSet& mixset = mixer->getMixSet();
    MixSet& mset = const_cast<MixSet&>(mixset);
    for( MixDevice* md = mset.first(); md != 0; md = mset.next() )
    {
            //kdDebug(67100) << "DialogSelectMaster::createPage() mset append qrb" << endl;
            QString mdName = md->name();
	    mdName.replace('&', "&&"); // Quoting the '&' needed, to prevent QRadioButton creating an accelerator
	    QRadioButton* qrb = new QRadioButton( mdName, m_vboxForScrollView);
	    m_buttonGroupForScrollView->insert(qrb, md->num());
	    //_qEnabledCB.append(qrb);
	    qrb->setChecked(false); // cannot match the current master at the moment.
	    //cb->setChecked( !mdw->isDisabled() ); // !! Select the currently active item
    }

    //m_vboxForScrollView->resize(m_vboxForScrollView->sizeHint());
    m_vboxForScrollView->show();  // show() is neccesary starting with the second call to createPage()
    //resize(m_vboxForScrollView->sizeHint() );
}


void DialogSelectMaster::apply()
{
   int soundcard_id = 0;
   if ( Mixer::mixers().count() > 1 ) {
     soundcard_id = m_cMixer->currentItem();
   }
   int channel_id = m_buttonGroupForScrollView->selectedId();
   if ( channel_id != -1 ) {
     // A channel was selected by the user => emit the "newMasterSelected()" signal
     kdDebug(67100) << "DialogSelectMaster::apply(): card=" << soundcard_id << ", channel=" << channel_id << endl;
     emit newMasterSelected(soundcard_id, channel_id);
   }
}

#include "dialogselectmaster.moc"

