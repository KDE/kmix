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

#ifndef KMIXERWIDGET_H
#define KMIXERWIDGET_H

#include <kpanelapplet.h>

#include <qwidget.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlist.h>
#include <qstring.h>

#include "channel.h"

class Mixer;
class QSlider;
class Channel;
class KActionCollection;
class KConfig;


class KMixerWidget : public QWidget  {
   Q_OBJECT

  public:
   KMixerWidget( int _id, Mixer *mixer, QString mixerName, int mixerNum,
                 bool small, KPanelApplet::Direction,
                 QWidget *parent=0, const char *name=0 );
   ~KMixerWidget();

   QString name() { return m_name; };
   void setName( QString name ) { m_name = name; };

   Mixer *mixer() { return m_mixer; };
   QString mixerName() { return m_mixerName; };
   int mixerNum() { return m_mixerNum; };

   int id() { return m_id; };

   struct Colors {      
       QColor high, low, back, mutedHigh, mutedLow, mutedBack;
   };

  signals:
   void updateLayout();

  public slots:     
   void setTicks( bool on );
   void setLabels( bool on );
   void setIcons( bool on );
   void setColors( const Colors &color );
    
   void saveConfig( KConfig *config, QString grp );
   void loadConfig( KConfig *config, QString grp );

   void showAll();

  private slots:	
   void rightMouseClicked();	
   void updateSize();

  private:
   Mixer *m_mixer;
   QSlider *m_balanceSlider;
   QBoxLayout *m_topLayout;
   QBoxLayout *m_devLayout;
   QList<Channel> m_channels;

   QString m_name;
   QString m_mixerName;
   int m_mixerNum;
   int m_id;

   KActionCollection *m_actions;

   bool m_small;
   KPanelApplet::Direction m_direction;
   bool m_iconsEnabled;
   bool m_labelsEnabled;
   bool m_ticksEnabled;

   void mousePressEvent( QMouseEvent *e );
   void createDeviceWidgets( KPanelApplet::Direction );
   void createMasterVolWidget(KPanelApplet::Direction);
};

#endif
