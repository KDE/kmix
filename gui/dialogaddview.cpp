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

#include <qbuttongroup.h>
#include <QLabel>
#include <qradiobutton.h>
#include <qscrollarea.h>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kcombobox.h>
#include <kdebug.h>
#include <klocale.h>

#include "core/mixdevice.h"
#include "core/mixer.h"


QStringList DialogAddView::viewNames;
QStringList DialogAddView::viewIds;


DialogAddView::DialogAddView(QWidget* parent, Mixer *mixer  )
  : KDialog( parent )
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

    setCaption( i18n( "Add View" ) );
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

DialogAddView::~DialogAddView()
{
   delete _layout;
   delete m_vboxForScrollView;
}

/**
 * Create basic widgets of the Dialog.
 */
void DialogAddView::createWidgets(Mixer *ptr_mixer)
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
    
        QLabel *qlbl = new QLabel( i18n("Select mixer:"), m_mainFrame );
        mixerNameLayout->addWidget(qlbl);
        qlbl->setFixedHeight(qlbl->sizeHint().height());
    
        m_cMixer = new KComboBox( false, m_mainFrame);
        m_cMixer->setObjectName( QLatin1String( "mixerCombo" ) );
        m_cMixer->setFixedHeight(m_cMixer->sizeHint().height());
        connect( m_cMixer, SIGNAL(activated(int)), this, SLOT(createPageByID(int)) );

        for( int i =0; i<Mixer::mixers().count(); i++ )
        {
            Mixer *mixer = (Mixer::mixers())[i];
            m_cMixer->addItem( mixer->readableName() );
         } // end for all_Mixers
        // Make the current Mixer the current item in the ComboBox
        int findIndex = m_cMixer->findText( ptr_mixer->readableName() );
        if ( findIndex != -1 ) m_cMixer->setCurrentIndex( findIndex );
        
    
        m_cMixer->setToolTip( i18n("Current mixer" ) );
        mixerNameLayout->addWidget(m_cMixer);
    
    } // end if (more_than_1_Mixer)

    
    if ( Mixer::mixers().count() > 0 ) {
        QLabel *qlbl = new QLabel( i18n("Select the design for the new view:"), m_mainFrame );
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
void DialogAddView::createPageByID(int mixerId)
{
  //kDebug(67100) << "DialogAddView::createPage()";
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
 *              TODO: The mixer's backend MUST be inspected to find out the supported profiles.
 */
void DialogAddView::createPage(Mixer* mixer)
{
    /** --- Reset page -----------------------------------------------
     * In case the user selected a new Mixer via m_cMixer, we need
     * to remove the stuff created on the last call.
     */
    // delete the VBox. This should automatically remove all contained QRadioButton's.
    delete m_vboxForScrollView;
    delete m_scrollableChannelSelector;
    delete m_buttonGroupForScrollView;
    enableButton(Ok, false);

    
    /** Reset page end -------------------------------------------------- */
    
    m_buttonGroupForScrollView = new QButtonGroup(this); // invisible QButtonGroup

    m_scrollableChannelSelector = new QScrollArea(m_mainFrame);
    _layout->addWidget(m_scrollableChannelSelector);

    m_vboxForScrollView = new KVBox();


    for( int i=0; i<viewNames.size(); ++i )
    {
        // Create a RadioButton for each view type
        QString name = viewNames.at(i);
        name.replace('&', "&&"); // Quoting the '&' needed, to prevent QRadioButton creating an accelerator
        QRadioButton* qrb = new QRadioButton( name, m_vboxForScrollView);
        connect( qrb, SIGNAL(toggled(bool)), this, SLOT(profileRbtoggled(bool)) );

        qrb->setObjectName(viewIds.at(i));  // The object name is used as ID here: see apply()
        m_buttonGroupForScrollView->addButton(qrb);
    }

    m_scrollableChannelSelector->setWidget(m_vboxForScrollView);
    m_vboxForScrollView->show();  // show() is necessary starting with the second call to createPage()
}


void DialogAddView::profileRbtoggled(bool selected)
{
    if ( selected)
        enableButton(Ok, true);
}

void DialogAddView::apply()
{
    Mixer *mixer = 0;
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
   
    QAbstractButton* button =  m_buttonGroupForScrollView->checkedButton();
    if ( button != 0 ) {
      QString viewName = button->objectName();
      if ( mixer == 0 ) {
         kError(67100) << "DialogAddView::createPage(): Invalid Mixer (mixer=0)" << endl;
         return; // can not happen
      }
      else {
          kDebug() << "We should now create a new view " << viewName << " for mixer " << mixer->id();
          resultMixerId = mixer->id();
          resultViewName = viewName;
      }
   }
}

#include "dialogaddview.moc"

