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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <iostream>

#include <qcursor.h>
#include <qstring.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qslider.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qwidgetstack.h>
#include <qmap.h>
#include <qsize.h>
#include <qhbox.h>

#include <kcombobox.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kglobalaccel.h>
#include <kdialog.h>
#include <ktabwidget.h>

#include "kmixerwidget.h"
#include "mixer.h"
#include "mixdevicewidget.h"

KMixerWidget::KMixerWidget( int _id, Mixer *mixer, const QString &mixerName, int mixerNum,
                            bool small, KPanelApplet::Direction dir, MixDevice::DeviceCategory categoryMask,
                            QWidget * parent, const char * name )
   : QWidget( parent, name ), m_mixer(mixer), m_balanceSlider(0),
     m_topLayout(0), m_appletLayout(0),
     m_name( mixerName ), m_mixerName( mixerName ), m_mixerNum( mixerNum ), m_id( _id ),
     m_direction( dir ),
     m_iconsEnabled( true ), m_labelsEnabled( false ), m_ticksEnabled( false )

{
   m_actions = new KActionCollection( this );

   m_categoryMask = categoryMask;

   m_toggleMixerChannels = new KActionMenu(i18n("&Channels"), m_actions, "toggle_channels");

   connect(m_toggleMixerChannels->popupMenu(), SIGNAL(aboutToShow()),
      this, SLOT(slotFillPopup()));
   connect(m_toggleMixerChannels->popupMenu(), SIGNAL(activated(int)),
      this, SLOT(slotToggleMixerDevice(int)));

   m_channels.setAutoDelete( true );
   m_small = small;

   // Create mixer device widgets
   if ( mixer )
   {
      createLayout();
   }
   else
   {
   		// No mixer found
      QBoxLayout *layout = new QHBoxLayout( this );
      QString s = i18n("Invalid mixer");
      if ( !mixerName.isEmpty() )
	 s += " \"" + mixerName + "\"";
      QLabel *errorLabel = new QLabel( s, this );
      errorLabel->setAlignment( QLabel::AlignCenter | QLabel::WordBreak );
      layout->addWidget( errorLabel );
   }
}

KMixerWidget::~KMixerWidget()
{
}

void
KMixerWidget::createLayout()
{
   if ( !m_mixer ) return;

   // delete old objects
   m_channels.clear();
   if( m_balanceSlider )
      delete m_balanceSlider;

   if( ! m_small )
   {

   	if( m_topLayout )
	{
		delete m_topLayout;
	}
 
      // create main layout
      m_topLayout = new QVBoxLayout( this, 0, 3,  "m_topLayout" );
 
      // Create tabs e widgetstack
      m_ioTab = new KTabWidget( this, "ioTab" );
      m_topLayout->add( m_ioTab );

      // Create switch buttonGroup
      m_swWidget = new QWidget( this, "switchWidget" );
      m_swWidget->hide(); // hidden, until the user clicks the "Advanced/Expand" button.
      m_devSwitchLayout = new QGridLayout( m_swWidget, 0, 0, 0, 0,"devSwitchLayout" );

      // Both Layouts and widgets
      m_oWidget = new QHBox( m_ioTab, "OutputTab" );
      m_iWidget = new QHBox( m_ioTab, "InputTab" );

      m_ioTab->addTab( m_oWidget, i18n("Output") );
      m_ioTab->addTab( m_iWidget, i18n("Input" ) );

	// Create device widgets
		createDeviceWidgets();

   }
   else
   {
   		// This is called for the small version (PanelApplet)
   		// Bad. With the current design, users will not see recordable devices, e.g. PCM, CD, ...
   		// This is because "Output" currently means: "can be recorded"
      m_oWidget = new QHBox( this, "OutputTab" );
      m_appletLayout = new QHBoxLayout( this, 0, 0 );
      m_appletLayout->add( m_oWidget );
		createDeviceWidgets();
   }
}

void
KMixerWidget::createDeviceWidgets()
{
   int row=0, col=0;
   // create devices
   MixSet mixSet = m_mixer->getMixSet();
   MixDevice *mixDevice = mixSet.first();
   for ( ; mixDevice != 0; mixDevice = mixSet.next())
   {
      MixDeviceWidget *mdw;
      if ( mixDevice->isSwitch() )
      {
         if( ! m_small )
            mdw = new MixDeviceWidget( m_mixer,  mixDevice, !m_small, !m_small, m_small,
                                       m_direction, m_swWidget, this, mixDevice->name().latin1() );
         else
            continue;
      }
      else if( ! mixDevice->isRecordable() )
      {
         mdw = new MixDeviceWidget( m_mixer,  mixDevice, !m_small, !m_small, m_small,
                                    m_direction, m_oWidget, this, mixDevice->name().latin1() );
      }
      else
      {
         if( ! m_small )
            mdw = new MixDeviceWidget( m_mixer,  mixDevice, !m_small, !m_small, m_small,
                                       m_direction, m_iWidget, this, mixDevice->name().latin1() );
         else
            continue;
      }

      connect( mdw, SIGNAL( newMasterVolume(Volume) ), SIGNAL( newMasterVolume(Volume) ) );
      connect( mdw, SIGNAL( updateLayout() ), this, SLOT(updateSize()));
      connect( mdw, SIGNAL( masterMuted( bool ) ), SIGNAL( masterMuted( bool ) ) );

      if( ! m_small )
      {
         if( mixDevice->isSwitch() )
         {
            m_devSwitchLayout->addWidget( mdw, row, col );
            col++;

				if( col > 3 )
				{
					col = 0;
					row++;
				}
			}
      }

      Channel *chn = new Channel;
      chn->dev = mdw;
      m_channels.append( chn );
   }

   if ( !m_small )
   {
		QHBoxLayout *balanceAndDetail = new QHBoxLayout( m_topLayout, 8,  "balanceAndDetail");

   	// Create the left-right-slider
      m_balanceSlider = new QSlider( -100, 100, 25, 0, QSlider::Horizontal,
                                     this, "RightLeft" );
      m_balanceSlider->setTickmarks( QSlider::Below );
      m_balanceSlider->setTickInterval( 25 );
      m_balanceSlider->setMinimumSize( m_balanceSlider->sizeHint() );
     // if( ! m_small ) // superfluous

        QLabel *mixerName = new QLabel(this, "mixerName");
        mixerName->setText( m_mixer->mixerName() );

        QCheckBox *hideShowDetail = new QCheckBox(this, "hideShowDetail");
        hideShowDetail->setChecked(false);
        //hideShowDetail->setToggleButton(true);
        hideShowDetail->setText( i18n("Advanced") );

		balanceAndDetail->addSpacing( 10 );
		balanceAndDetail->addWidget( hideShowDetail);
		balanceAndDetail->addWidget( m_balanceSlider );
		balanceAndDetail->addWidget( mixerName );
		balanceAndDetail->addSpacing( 10 );

 		//m_topLayout->addLayout( balanceAndDetail );
      connect( m_balanceSlider, SIGNAL(valueChanged(int)), m_mixer, SLOT(setBalance(int)) );
      connect( hideShowDetail, SIGNAL(toggled(bool)), this, SLOT(hideShowDetail(bool)) );
      QToolTip::add( m_balanceSlider, i18n("Left/Right balancing") );

      // Add the Switch widget
      m_topLayout->addWidget( m_swWidget );
   }
   else
	{
		m_balanceSlider = 0;
	}

	updateSize(true);
	// we have to expliciteley set the size, as
}

void
KMixerWidget::updateSize()
{
	updateSize(false);
}

void
KMixerWidget::updateSize(bool force)
{
	//kdDebug() << "KMixerWidget::updateSize(): (" << layout()->minimumSize().width() << "," << layout()->minimumSize().height() << ")\n";
   layout()->activate();
   setMinimumWidth( layout()->minimumSize().width() );
   setMinimumHeight( layout()->minimumSize().height() );
   if ( force ) {
		resize( layout()->minimumSize().width(), layout()->minimumSize().height() );
   }
   emit updateLayout();
}

void
KMixerWidget::setTicks( bool on )
{
   if ( m_ticksEnabled!=on )
   {
      m_ticksEnabled = on;
      for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() )
         chn->dev->setTicks( on );
   }
   updateSize(false);
}

void
KMixerWidget::setLabels( bool on )
{
   if ( m_labelsEnabled!=on )
   {
      m_labelsEnabled = on;
      for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() )
         chn->dev->setLabeled( on );
   }
   updateSize(false);
}

void
KMixerWidget::setIcons( bool on )
{
   if ( m_iconsEnabled!=on )
   {
      m_iconsEnabled = on;
      for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() )
         chn->dev->setIcons( on );
   }
}

void KMixerWidget::hideShowDetail(bool on) {
	if ( on ) {
			m_swWidget->show();
			updateSize(false); // don't force new size (will make automatically bigger if neccesary)
	}
	else {
			m_swWidget->hide();
			updateSize(true); //
	}
}

void
KMixerWidget::setColors( const Colors &color )
{
    for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() ) {
        chn->dev->setColors( color.high, color.low, color.back );
        chn->dev->setMutedColors( color.mutedHigh, color.mutedLow, color.mutedBack );
    }
}

void
KMixerWidget::mousePressEvent( QMouseEvent *e )
{
   if ( e->button()==RightButton )
      rightMouseClicked();
}

void
KMixerWidget::addActionToPopup( KAction *action )
{
   m_actions->insert( action );

   for ( Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next() )
   {
      chn->dev->addActionToPopup( action );
   }
}

KPopupMenu*
KMixerWidget::getPopup()
{
   popupReset();
   return m_popMenu;
}

void
KMixerWidget::popupReset()
{
   KAction *a;

   m_popMenu = new KPopupMenu( this );
   m_popMenu->insertTitle( SmallIcon( "kmix" ), i18n("Device Settings") );

   a = m_actions->action( "toggle_channels" );
   if ( a ) a->plug( m_popMenu );

   a = m_actions->action( "options_show_menubar" );
   if ( a ) a->plug( m_popMenu );
}

void
KMixerWidget::rightMouseClicked()
{
	popupReset();

   QPoint pos = QCursor::pos();
   m_popMenu->popup( pos );
}

void
KMixerWidget::slotFillPopup()
{
	QStringList output, input, sw;
	QMap<QString, bool> state;
   int n=0;

   m_toggleMixerChannels->popupMenu()->clear();

   for (Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next())
   {
		if( chn->dev->isSwitch() )
			sw << chn->dev->name();
		else if ( chn->dev->isRecsrc() )
			input << chn->dev->name();
		else
			output << chn->dev->name();

		state[ chn->dev->name() ] = !chn->dev->isDisabled();
   }

	// Output
	m_toggleMixerChannels->popupMenu()->insertTitle(  SmallIcon(  "kmix" ), i18n( "Output" ) );
	for ( QStringList::Iterator it = output.begin(); it != output.end(); ++it )
	{
		m_toggleMixerChannels->popupMenu()->insertItem( *it, n );
		m_toggleMixerChannels->popupMenu()->setItemChecked( n, state[ *it ] );
		n++;
	}

	// Input
	m_toggleMixerChannels->popupMenu()->insertTitle(  SmallIcon(  "kmix" ), i18n( "Input" ) );
	for ( QStringList::Iterator it = input.begin(); it != input.end(); ++it )
	{
		m_toggleMixerChannels->popupMenu()->insertItem( *it, n );
		m_toggleMixerChannels->popupMenu()->setItemChecked( n, state[ *it ] );
		n++;
	}

	// Switch
	m_toggleMixerChannels->popupMenu()->insertTitle(  SmallIcon(  "kmix" ), i18n( "Switches" ) );
	for ( QStringList::Iterator it = sw.begin(); it != sw.end(); ++it )
	{
		m_toggleMixerChannels->popupMenu()->insertItem( *it, n );
		m_toggleMixerChannels->popupMenu()->setItemChecked( n, state[ *it ] );
		n++;
	}

}

void
KMixerWidget::slotToggleMixerDevice( int id)
{
   if(id >= static_cast<int>(m_channels.count())) // too big
      return;

   Channel *chn = m_channels.at(id);
   if(!chn)
      return;

   bool gotCheck = m_toggleMixerChannels->popupMenu()->isItemChecked(id);

   chn->dev->setDisabled(gotCheck);
   m_toggleMixerChannels->popupMenu()->setItemChecked(id, !gotCheck);
}

void
KMixerWidget::saveConfig( KConfig *config, const QString &grp ) const
{
   config->setGroup( grp );

   config->writeEntry( "Devs", m_channels.count() );
   config->writeEntry( "Name", m_name );

   int n=0;
   for (Channel *chn=m_channels.first(); chn!=0; chn=m_channels.next())
   {
      QString devgrp;
      devgrp.sprintf( "%s.Dev%i", grp.ascii(), n );
      config->setGroup( devgrp );

      config->writeEntry( "Split", !chn->dev->isStereoLinked() );
      config->writeEntry( "Show", !chn->dev->isDisabled() );

      KGlobalAccel *keys=chn->dev->keys();
      if (keys) {
         QString devgrpkeys;
         devgrpkeys.sprintf( "%s.Dev%i.keys", grp.ascii(), n );
         keys->setConfigGroup(devgrpkeys);
         keys->writeSettings(config);
      }
      n++;
   }
}

void
KMixerWidget::loadConfig( KConfig *config, const QString &grp )
{
	config->setGroup( grp );

	int num = config->readNumEntry("Devs", 0);
	m_name = config->readEntry("Name", m_name );

	int n=0;
	for (Channel *chn=m_channels.first(); chn!=0 && n<num; chn=m_channels.next())
	{
		QString devgrp;
		devgrp.sprintf( "%s.Dev%i", grp.ascii(), n );
		config->setGroup( devgrp );

		chn->dev->setStereoLinked( !config->readBoolEntry("Split", false) );
		chn->dev->setDisabled( !config->readBoolEntry("Show", true) );

		KGlobalAccel *keys=chn->dev->keys();
		if ( keys )
		{
			QString devgrpkeys;
			devgrpkeys.sprintf( "%s.Dev%i.keys", grp.ascii(), n );

			keys->setConfigGroup(devgrpkeys);
			keys->readSettings(config);
			keys->updateConnections();
		}

		n++;
	}
}

/**
 * Updates the Balance slider (GUI).
 */
void
KMixerWidget::updateBalance()
{
  MixDevice *md = m_mixer->mixDeviceByType( 0 );
  if (!md) return;
  int right= md->rightVolume();
  int left = md->leftVolume();

  int value = 0;
  if ( left != right ) {
    int refvol = left > right ? left : right;
    if (left > right)
      value = (100*right) / refvol - 100;
    else
      value = -((100*left) / refvol - 100);
  }
  m_balanceSlider->blockSignals( true );
  m_balanceSlider->setValue( value );
  m_balanceSlider->blockSignals( false );
}

#include "kmixerwidget.moc"
