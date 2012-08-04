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

#include "gui/kmixprefdlg.h"

#include <qbuttongroup.h>
#include <qwhatsthis.h>
#include <QCheckBox>
#include <QLabel>
#include <qradiobutton.h>

#include <kapplication.h>
#include <klocale.h>

#include "gui/kmixerwidget.h"


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
   layout->setMargin( 0 );
   layout->setSpacing( KDialog::spacingHint() );

// -----------------------------------------------------------

   QLabel *label;
   label = new QLabel( i18n("Behavior"), m_generalTab );
   layout->addWidget(label);

   QBoxLayout *l;
   l = new QHBoxLayout();
   layout->addItem( l );
      l->addSpacing(10);
      m_dockingChk = new QCheckBox( i18n("&Dock in system tray"), m_generalTab );
      l->addWidget( m_dockingChk );
      m_dockingChk->setWhatsThis( i18n("Docks the mixer into the KDE system tray"));

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
         m_beepOnVolumeChange = new QCheckBox( i18n("Volume Feedback"), m_generalTab );
         l->addWidget( m_beepOnVolumeChange );

   l = new QHBoxLayout();
   l->addSpacing(10);
   layout->addItem( l );
       volumeFeedbackWarning = new QLabel( i18n("Volume feedback is only available for Pulseaudio."), m_generalTab );
      volumeFeedbackWarning->setEnabled(false);
      l->addWidget(volumeFeedbackWarning);


   label = new QLabel( i18n("Startup"), m_generalTab );
   layout->addWidget(label);

   l = new QHBoxLayout();
   layout->addItem( l );
      l->addSpacing(10);
      m_onLogin = new QCheckBox( i18n("Restore volumes on login"), m_generalTab );
      m_onLogin->setToolTip(i18n("Restore all volume levels and switches."));
      l->addWidget( m_onLogin );

   l = new QHBoxLayout();
   l->addSpacing(10);
   layout->addItem( l );

      dynamicControlsRestoreWarning = new QLabel( i18n("Dynamic controls from Pulseaudio and MPRIS2 will not be restored."), m_generalTab );
      dynamicControlsRestoreWarning->setEnabled(false);
/*      QWidget *spacer = new QWidget();
      spacer->setFixedWidth(200);
      spacer->setMinimumHeight(2);
      l->addWidget(spacer);
      */
      l->addWidget(dynamicControlsRestoreWarning);

   l = new QHBoxLayout();
   layout->addItem( l );
      l->addSpacing(10);
      m_supressAutostart = new QCheckBox( i18n("Supress KMix Autostart"), m_generalTab );
      m_supressAutostart->setToolTip(i18n("Disables the KMix autostart service (kmix_autostart.desktop)"));
      l->addWidget( m_supressAutostart );


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
      QLabel* qlb = new QLabel( i18n("Slider orientation: "), m_generalTab );
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

void KMixPrefDlg::showEvent ( QShowEvent * event )
{
  // As GUI can change, the warning will only been shown on demand
  dynamicControlsRestoreWarning->setVisible(Mixer::dynamicBackendsPresent());
  
  // Pulseaudio supports volume feedback. Disable the configuaration option for all other backends
  // and show a warning.
  bool volumeFeebackAvailable = Mixer::pulseaudioPresent();
  volumeFeedbackWarning->setVisible(!volumeFeebackAvailable);
  m_beepOnVolumeChange->setDisabled(!volumeFeebackAvailable);
  KDialog::showEvent(event);
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
