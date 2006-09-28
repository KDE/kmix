/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
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

#include <qbuttongroup.h>
#include <QLayout>
#include <qwhatsthis.h>
#include <QCheckBox>
#include <QLabel>
#include <qradiobutton.h>

#include <klocale.h>
// For "kapp"
#include <kapplication.h>

#include "kmix.h"
#include "kmixprefdlg.h"
#include "kmixerwidget.h"


KMixPrefDlg::KMixPrefDlg( QWidget *parent )
    : KDialog( parent )
{
    setCaption( i18n( "Configure" ) );
    setButtons( Ok|Cancel|Apply );
    setDefaultButton( Ok );

   // general buttons
   m_generalTab = new QFrame( this );
   setMainWidget( m_generalTab );

   QBoxLayout *layout = new QVBoxLayout( m_generalTab );
   layout->setSpacing( KDialog::spacingHint() );

// -----------------------------------------------------------

   QLabel *label;
   label = new QLabel( i18n("Behaviour"), m_generalTab );
   layout->addWidget(label);

   QBoxLayout *l;
   l = new QHBoxLayout();
   layout->addItem( l );
      l->addSpacing(10);
      m_dockingChk = new QCheckBox( i18n("&Dock into panel"), m_generalTab );
      l->addWidget( m_dockingChk );
      m_dockingChk->setWhatsThis( i18n("Docks the mixer into the KDE panel"));

      l = new QHBoxLayout();
      layout->addItem( l );
         l->addSpacing(20);
         m_volumeChk = new QCheckBox(i18n("Enable system tray &volume control"), m_generalTab);
         l->addWidget(m_volumeChk);
         m_volumeChk->setWhatsThis( i18n("Allows to control the volume from the system tray"));
         connect(m_dockingChk, SIGNAL(stateChanged(int)), SLOT(dockIntoPanelChange(int)) );

   l = new QHBoxLayout();
   layout->addItem( l );
      l->addSpacing(10);
      m_onLogin = new QCheckBox( i18n("Restore volumes on login"), m_generalTab );
      l->addWidget( m_onLogin );

// -----------------------------------------------------------

   label = new QLabel( i18n("Visual"), m_generalTab );
   layout->addWidget(label);

   l = new QHBoxLayout();
   layout->addItem( l );
      l->addSpacing(10);
      m_showTicks = new QCheckBox( i18n("Show &tickmarks"), m_generalTab );
      l->addWidget( m_showTicks );
      m_showTicks->setWhatsThis( i18n("Enable/disable tickmark scales on the sliders"));

   l = new QHBoxLayout();
   layout->addItem( l );
      l->addSpacing(10);
      m_showLabels = new QCheckBox( i18n("Show &labels"), m_generalTab );
      l->addWidget( m_showLabels );
      m_showLabels->setWhatsThis( i18n("Enables/disables description labels above the sliders"));

   QBoxLayout *orientationLayout = new QHBoxLayout();
      orientationLayout->addSpacing(10);
      layout->addItem( orientationLayout );
      QButtonGroup* orientationGroup = new QButtonGroup( m_generalTab );
      orientationGroup->setExclusive(true);
      QLabel* qlb = new QLabel( i18n("Slider Orientation: "), m_generalTab );
      _rbHorizontal = new QRadioButton(i18n("&Horizontal"), m_generalTab );
      _rbVertical   = new QRadioButton(i18n("&Vertical"  ), m_generalTab );
      orientationGroup->addButton(_rbHorizontal);
      orientationGroup->addButton(_rbVertical);
      
      orientationLayout->addWidget(qlb);
      orientationLayout->addWidget(_rbHorizontal);
      orientationLayout->addWidget(_rbVertical);
      
      orientationLayout->addStretch();
      layout->addStretch();
      showButtonSeparator(true);

   connect( this, SIGNAL(applyClicked()), this, SLOT(apply()) );
   connect( this, SIGNAL(okClicked()), this, SLOT(apply()) );
}

KMixPrefDlg::~KMixPrefDlg()
{
}

void KMixPrefDlg::apply()
{
   // disabling buttons => users sees that we are working
   enableButtonOk(false);
   enableButtonCancel(false);
   enableButtonApply(false);
   kapp->processEvents();
   emit signalApplied( this );
   // re-enable (in case of "Apply")
   enableButtonOk(true);
   enableButtonCancel(true);
   enableButtonApply(true);
}

void KMixPrefDlg::dockIntoPanelChange(int state)
{
   if ( state == Qt::Unchecked ) {
      m_volumeChk->setDisabled(true);
   } else {
     m_volumeChk->setEnabled(true);
   } 
}

#include "kmixprefdlg.moc"
