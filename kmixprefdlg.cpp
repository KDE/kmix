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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qbuttongroup.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qradiobutton.h>

#include <klocale.h>
// For "kapp"
#include <kapplication.h>

#include "kmix.h"
#include "kmixprefdlg.h"
#include "kmixerwidget.h"


KMixPrefDlg::KMixPrefDlg( QWidget *parent )
    : KDialogBase(  Plain, i18n( "Configure" ),
          Ok|Cancel|Apply, Ok, parent )
{
   // general buttons
   m_generalTab = plainPage( /* i18n("&General") */ );

   QBoxLayout *layout = new QVBoxLayout( m_generalTab );
   layout->setSpacing( KDialog::spacingHint() );

   m_dockingChk = new QCheckBox( i18n("&Dock into panel"), m_generalTab );
   layout->addWidget( m_dockingChk );
   QWhatsThis::add(m_dockingChk, i18n("Docks the mixer into the KDE panel"));

   m_volumeChk = new QCheckBox(i18n("Enable system tray &volume control"),
			       m_generalTab);
   layout->addWidget(m_volumeChk);

   m_showTicks = new QCheckBox( i18n("Show &tickmarks"), m_generalTab );
   layout->addWidget( m_showTicks );
   QWhatsThis::add(m_showTicks,
           i18n("Enable/disable tickmark scales on the sliders"));

   m_showLabels = new QCheckBox( i18n("Show &labels"), m_generalTab );
   layout->addWidget( m_showLabels );
   QWhatsThis::add(m_showLabels,
           i18n("Enables/disables description labels above the sliders"));


   m_onLogin = new QCheckBox( i18n("Restore volumes on login"), m_generalTab );
   layout->addWidget( m_onLogin );

   QBoxLayout *numbersLayout = new QHBoxLayout( layout );
   QButtonGroup *numbersGroup = new QButtonGroup( 3, Qt::Horizontal, i18n("Numbers"), m_generalTab );
   numbersGroup->setRadioButtonExclusive(true);
   QLabel* qlbl = new QLabel(  i18n("Volume Values: "), m_generalTab );
   _rbNone = new QRadioButton( i18n("&None"), m_generalTab );
   _rbAbsolute = new QRadioButton( i18n("A&bsolute"), m_generalTab );
   _rbRelative   = new QRadioButton( i18n("&Relative"), m_generalTab );
   numbersGroup->insert(_rbNone);
   numbersGroup->insert(_rbAbsolute);
   numbersGroup->insert(_rbRelative);
   numbersGroup->hide();

   numbersLayout->add(qlbl);
   numbersLayout->add(_rbNone);
   numbersLayout->add(_rbAbsolute);
   numbersLayout->add(_rbRelative);
   numbersLayout->addStretch();

   QBoxLayout *orientationLayout = new QHBoxLayout( layout );
   QButtonGroup* orientationGroup = new QButtonGroup( 2, Qt::Horizontal, i18n("Orientation"), m_generalTab );
   //orientationLayout->add(orientationGroup);
   orientationGroup->setRadioButtonExclusive(true);
   QLabel* qlb = new QLabel( i18n("Slider Orientation: "), m_generalTab );
   _rbHorizontal = new QRadioButton(i18n("&Horizontal"), m_generalTab );
   _rbVertical   = new QRadioButton(i18n("&Vertical"  ), m_generalTab );
   orientationGroup->insert(_rbHorizontal);
   orientationGroup->insert(_rbVertical);
   orientationGroup->hide();
   //orientationLayout->add(qlb);
   //orientationLayout->add(orientationGroup);

   orientationLayout->add(qlb);
   orientationLayout->add(_rbHorizontal);
   orientationLayout->add(_rbVertical);

   orientationLayout->addStretch();
   layout->addStretch();
   enableButtonSeparator(true);

   connect( this, SIGNAL(applyClicked()), this, SLOT(apply()) );
   connect( this, SIGNAL(okClicked()), this, SLOT(apply()) );
}

KMixPrefDlg::~KMixPrefDlg()
{
}

void KMixPrefDlg::apply()
{
   // disabling buttons => users sees that we are working
   enableButtonOK(false);
   enableButtonCancel(false);
   enableButtonApply(false);
   kapp->processEvents();
   emit signalApplied( this );
   // re-enable (in case of "Apply")
   enableButtonOK(true);
   enableButtonCancel(true);
   enableButtonApply(true);
}

#include "kmixprefdlg.moc"
