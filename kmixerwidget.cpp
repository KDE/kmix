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
#include <stdlib.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtimer.h>
#include <qslider.h>
#include <qtooltip.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qcombobox.h>

#include <klocale.h>
#include <kmessagebox.h>

#include "kmixerwidget.h"
#include "mixer.h"
#include "mixdevice.h"

KMixerWidget::KMixerWidget( Mixer *mixer, QWidget * parent, const char * name )
   : QWidget( parent, name ), m_mixer(mixer)
{
   cerr << "KMixerWidget::KMixerWidget" << endl;
   
   ASSERT(m_mixer!=0);
   
   // Create update timer
   m_timer = new QTimer;
   m_timer->start( 1000 );
   connect( m_timer, SIGNAL(timeout()), m_mixer, SLOT(readSetFromHW()));

   // Create mixer device widgets
   m_topLayout = new QVBoxLayout( this, 0, 3 );
   QBoxLayout* layout = new QHBoxLayout( m_topLayout );
   MixSet mixSet = m_mixer->getMixSet();
   MixDevice *mixDevice = mixSet.first();
   for ( ; mixDevice != 0; mixDevice = mixSet.next())
   {
      cerr << "mixDevice = " << mixDevice << endl;
      MixDeviceWidget *mdw =  new MixDeviceWidget( mixDevice, true, true,
						   this, mixDevice->name() );
      layout->addWidget( mdw );
      connect( mdw, SIGNAL( newVolume( int, Volume )),
               m_mixer, SLOT( writeVolumeToHW( int, Volume ) ));
      connect( mdw, SIGNAL( newRecsrc(int, bool)),
               m_mixer, SLOT( setRecsrc(int, bool ) ));
      connect( m_mixer, SIGNAL( newRecsrc()),
               mdw, SLOT( updateRecsrc() ));
      connect( m_timer, SIGNAL(timeout()), mdw, SLOT(updateSliders()) );
      connect( this, SIGNAL(updateTicks(bool)), mdw, SLOT(updateTicks(bool)) );
      if( mixDevice->num()==m_mixer->masterDevice() )
	 connect( m_mixer, SIGNAL(newBalance(Volume)), mdw, SLOT(setVolume(Volume)));
      connect( mdw, SIGNAL(rightMouseClick()), this, SLOT(rightMouseClicked()));
   }

   // Create the left-right-slider
   m_balanceSlider = new QSlider( -100, 100, 25, 0, QSlider::Horizontal,
				  this, "RightLeft" );
   m_topLayout->addWidget( m_balanceSlider );
   connect( m_balanceSlider, SIGNAL(valueChanged(int)), this, SLOT(setBalance(int)) );
   QToolTip::add( m_balanceSlider, i18n("Left/Right balancing") );

   updateSize();
}

KMixerWidget::~KMixerWidget()
{
   if (m_timer) delete m_timer;
}

QString KMixerWidget::mixerName()
{
   return m_mixer->mixerName();
}

void KMixerWidget::updateSize()
{
   setFixedWidth( m_topLayout->minimumSize().width() );
   setMinimumHeight( m_topLayout->minimumSize().height() );
}

void KMixerWidget::applyPrefs( KMixPrefWidget *prefWidget )
{
   updateSize();
}

void KMixerWidget::initPrefs( KMixPrefWidget *prefWidget )
{
}

void KMixerWidget::setTicks( bool on )
{
   emit updateTicks( on );
   updateSize();
}

void KMixerWidget::setBalance( int value )
{
   m_mixer->setBalance( value );
   m_balanceSlider->setValue( value );
}

void KMixerWidget::mousePressEvent( QMouseEvent *e )
{
   if ( e->button()==RightButton )
      emit rightMouseClick();
}

void KMixerWidget::sessionSave( bool sessionConfig )
{
   m_mixer->sessionSave( sessionConfig );
}

void KMixerWidget::sessionLoad( bool sessionConfig )
{
}


/********************************* KMixPrefWidget *****************************/

KMixerPrefWidget::KMixerPrefWidget( KMixerWidget* mixerWidget,
				    QWidget *parent, const char *name )
   : QWidget( parent, name )
{
   m_mixerWidget = mixerWidget;
   m_layout = new QVBoxLayout( this, 3, 3 );
   Mixer *mix = m_mixerWidget->m_mixer;
	
   // Add mixer name
   QLabel *mixerNameLabel = new  QLabel( mix->mixerName(), this);
   m_layout->addWidget(mixerNameLabel);

   // Add set selection Combo Box
   QBoxLayout *setLayout = new QHBoxLayout( m_layout, 3 );
   QComboBox *setSelectCombo = new QComboBox( this);
   setSelectCombo->insertItem(i18n("Kicker applet"));
   setSelectCombo->insertItem(i18n("Standard"));
   setLayout->addWidget( setSelectCombo );

   QPushButton *add = new QPushButton( i18n("Add"), this );
   QPushButton *remove = new QPushButton( i18n("Remove"), this );
   setLayout->addWidget( add );
   setLayout->addWidget( remove );
  
   // Channel selection box
   QGroupBox *box = new QGroupBox( i18n("Mixer channel setup"), this );
   m_layout->addWidget( box );

   QGridLayout *grid = new QGridLayout( box, 1, 3, 5 );
   int line = 0;
   grid->addRowSpacing( line, 10 );
   grid->setRowStretch( line++, 0 );

   QLabel *label = new QLabel( i18n("Device"), box );
   grid->addWidget( label, line, 0 );

   label = new QLabel( i18n("Show"), box );
   grid->addWidget( label, line, 1 );

   label = new QLabel( i18n("Split"), box );
   grid->addWidget( label, line, 2 );

   grid->setRowStretch( line++, 0 );
   grid->setRowStretch( line++, 1 );
	
   MixDevice *mixPtr;
   for ( unsigned int devNum = 0; devNum<mix->size(); devNum++ )
   {
      mixPtr = (*mix)[devNum];

      // 1. line edit
      QLineEdit *devNameEdt;
      devNameEdt = new QLineEdit(mixPtr->name(), box, mixPtr->name().ascii());
      grid->addWidget(devNameEdt, line, 0);

      // 2. check box  (Show)
      QCheckBox *showChk = new QCheckBox( box );
      grid->addWidget(showChk, line, 1);

#if 0 // remove soon
#warning This will be removed as soon as possible
      if (MixPtr->disabled())
	 showChk->setChecked(false);
      else
	 showChk->setChecked(true);
#endif

      // 3. check box  (Split)
      QCheckBox *splitChk;
      if (mixPtr->isStereo()) {
	 splitChk = new QCheckBox( box );

#if 0 // remove soon
#warning This will be removed as soon as possible
	 if (MixPtr->stereoLinked() )
	    splitChk->setChecked(false);
	 else
	    splitChk->setChecked(true);
#endif

	 grid->addWidget( splitChk, line, 2);
      }
      else
	 splitChk = NULL;

      grid->setRowStretch(line++, 0);
      grid->setRowStretch(line++, 1);

      m_channels.append(new ChannelSetup(mixPtr->num(), devNameEdt, showChk, splitChk));
   }
}

KMixerPrefWidget::~KMixerPrefWidget()
{
}
