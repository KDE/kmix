/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <iostream.h>

#include <qwidget.h>
#include <qtabdialog.h>
#include <qlayout.h>

#include <klocale.h>

#include "kmix.h"
#include "kmixprefdlg.h"
#include "kmixerwidget.h"
#include "kmixprefdlg.h"


KMixPrefDlg::KMixPrefDlg()
{
   setCaption( i18n("KMix Preferences") );

   // general buttons
   m_generalTab = new QWidget( this );
   QBoxLayout *layout = new QVBoxLayout( m_generalTab );

   m_dockingChk = new QCheckBox( i18n("&Dock into panel"), m_generalTab );
   layout->addWidget( m_dockingChk );

   m_hideOnCloseChk = new QCheckBox( i18n("Only &hide window with close button"), m_generalTab );
   layout->addWidget( m_hideOnCloseChk );

   m_showTicks = new QCheckBox( i18n("Show &tickmarks"), m_generalTab );
   layout->addWidget( m_showTicks );

   m_showLabels = new QCheckBox( i18n("Show &labels"), m_generalTab );
   layout->addWidget( m_showLabels );

   addTab( m_generalTab, i18n("&General") );

   // dialog buttons
   setCancelButton( i18n("&Cancel") );
   setOkButton( i18n("&OK") );
   setApplyButton( i18n("&Apply") );

   connect( this, SIGNAL(applyButtonPressed()), this, SLOT(apply()) );
}

KMixPrefDlg::~KMixPrefDlg()
{
}

void KMixPrefDlg::apply()
{
   cerr << "KMixPrefDlg::apply()" << endl;
   emit signalApplied( this );
}

#include "kmixprefdlg.moc"
