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

#include <qvbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qptrlist.h>
#include <qradiobutton.h>
#include <qscrollview.h>
#include <qtooltip.h>

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
   createPage(0);  // Open with Mixer Hardware #0

}

DialogSelectMaster::~DialogSelectMaster()
{
   delete _layout;
   _layout = 0;
}

void DialogSelectMaster::createPage(int mixerId)
{
    delete _layout; // Remove old layout (in case of a re-layout by selecting another soundcard).
    _layout = new QVBoxLayout(plainPage(),0,-1, "_layout" );

    
    //    kdDebug(67100) << "DialogSelectMaster::DialogSelectMaster add header" << "\n";

    if ( Mixer::mixers().count() > 1 ) {
      // More than one Mixer => show Combo-Box to select Mixer
      // Mixer widget line
      QHBoxLayout* mixerNameLayout = new QHBoxLayout( _layout );
      //widgetsLayout->setStretchFactor( mixerNameLayout, 0 );
      //QSizePolicy qsp( QSizePolicy::Ignored, QSizePolicy::Maximum);
      //mixerNameLayout->setSizePolicy(qsp);
      mixerNameLayout->setSpacing(KDialog::spacingHint());
      
      QLabel *qlbl = new QLabel( i18n("Current Mixer"), this );
      mixerNameLayout->addWidget(qlbl);
      qlbl->setFixedHeight(qlbl->sizeHint().height());
      
      KComboBox* m_cMixer = new KComboBox( FALSE, this, "mixerCombo" );
      m_cMixer->setFixedHeight(m_cMixer->sizeHint().height());
      connect( m_cMixer, SIGNAL( activated( int ) ), this, SLOT( createPage( int ) ) );
      for ( Mixer *mixer = Mixer::mixers().first(); mixer !=0; mixer = Mixer::mixers().next() ) {
	m_cMixer->insertItem( mixer->mixerName() );
      }
      m_cMixer->setCurrentItem(mixerId);
      QToolTip::add( m_cMixer, i18n("Current mixer" ) );
      mixerNameLayout->addWidget(m_cMixer);
      _layout->addLayout(mixerNameLayout);
      mixerNameLayout->activate();   // !!
    }

    Mixer *mixer = Mixer::mixers().at(mixerId);
    if ( mixer == 0 ) {
      kdDebug(67100) << "DialogSelectMaster::createPage(): Invalid Mixer (mixerID=" << mixerId << ")" << endl;
      return; // can not happen
    }
    
    QLabel *qlbl = new QLabel( i18n("Select Channel"), this );
    _layout->addWidget(qlbl);
    
    // --- Show devices of current mixer ---
    QScrollView* scrollableChannelSelector = new QScrollView(this, "scrollableChannelSelector");
    _layout->add(scrollableChannelSelector);
    QVBox* vboxForScrollView = new QVBox(scrollableChannelSelector->viewport());
    scrollableChannelSelector->addChild(vboxForScrollView);
    
    
    const MixSet& mixset = mixer->getMixSet();
    MixSet& mset = const_cast<MixSet&>(mixset);
    for( MixDevice* md = mset.first(); md != 0; md = mset.next() )
    {
	    QString mdName = md->name();
	    mdName.replace('&', "&&"); // Quoting the '&' needed, to prevent QRadioButton creating an accelerator
	    QRadioButton* qrb = new QRadioButton( mdName, vboxForScrollView );  // plainPage()
	    _qEnabledCB.append(qrb);
	    qrb->setChecked(false); // cannot match the current master at the moment.
	    //cb->setChecked( !mdw->isDisabled() ); //mdw->isVisible() );
	    //vboxForScrollView->addWidget(qrb);
    }
    _layout->activate();
    resize(_layout->sizeHint() );
    connect( this, SIGNAL(okClicked())   , this, SLOT(apply()) );
}


void DialogSelectMaster::apply()
{
   // emit parameters: soundcard_id, channel_id
   emit newMasterSelected(0,5);
}

#include "dialogselectmaster.moc"

