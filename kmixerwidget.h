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

#ifndef KMIXERWIDGET_H
#define KMIXERWIDGET_H

#include <qwidget.h>
#include <qptrlist.h>
class QString;
class QGridLayout;

#include <kpanelapplet.h>
class KPopupMenu;

#include "channel.h"
#include "mixer.h"
#include "mixdevicewidget.h"

// QT
class QSlider;


// KDE
class KActionCollection;
class KActionMenu;
class KConfig;
class KTabWidget;

// KMix
class Channel;
class Mixer;


class 
Channel
{
   public:
      Channel() : dev( 0 ) {};
      ~Channel() { delete dev; };

      MixDeviceWidget *dev;
};

class 
KMixerWidget : public QWidget  
{
   Q_OBJECT

  public:
   KMixerWidget( int _id, Mixer *mixer, QString mixerName, int mixerNum,
                 bool small, KPanelApplet::Direction, MixDevice::DeviceCategory categoryMask = MixDevice::ALL ,
                 QWidget *parent=0, const char *name=0 );
   ~KMixerWidget();
	
   enum KMixerWidgetIO { OUTPUT=0, INPUT };

   void addActionToPopup( KAction *action );

   QString name() const { return m_name; };
   void setName( QString name ) { m_name = name; };

   Mixer *mixer() const { return m_mixer; };
   QString mixerName()  const { return m_mixerName; };
   int mixerNum() const { return m_mixerNum; };

   int id() const { return m_id; };

   KPopupMenu* getPopup();
   void popupReset();

   KActionCollection* getActionCollection() const { return m_actions; }
	
   struct Colors {
       QColor high, low, back, mutedHigh, mutedLow, mutedBack;
   };

  signals:
   void updateLayout();
   void masterMuted( bool );
   void newMasterVolume(Volume vol);

  public slots:
   void setTicks( bool on );
   void setLabels( bool on );
   void setIcons( bool on );
   void setColors( const Colors &color );

   void saveConfig( KConfig *config, QString grp );
   void loadConfig( KConfig *config, QString grp );

  private slots:
   void rightMouseClicked();
   void updateBalance();
   void updateSize();
   void slotFillPopup();
   void slotToggleMixerDevice(int id);

  private:
   Mixer *m_mixer;
   QSlider *m_balanceSlider;
   QWidget *m_swWidget;
   QBoxLayout *m_topLayout;
   QWidget *m_iWidget;
   QWidget *m_oWidget;
   QGridLayout *m_devSwitchLayout;
	
   QPtrList<Channel> m_channels;
   KTabWidget *m_ioTab;

   QString m_name;
   QString m_mixerName;
   int m_mixerNum;
   int m_id;

   KActionCollection *m_actions;
   KActionMenu *m_toggleMixerChannels;
   KPopupMenu *m_popMenu;

   bool m_small;
   KPanelApplet::Direction m_direction;
   bool m_iconsEnabled;
   bool m_labelsEnabled;
   bool m_ticksEnabled;
   MixDevice::DeviceCategory m_categoryMask;

   void mousePressEvent( QMouseEvent *e );
   void createDeviceWidgets();
   void createLayout();
   void createMasterVolWidget(KPanelApplet::Direction);
};

#endif
