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

struct ChannelProfile
{       
};

class Profile : public QList<ChannelProfile>
{
  public:  
   Profile( Mixer *mixer );

   void write();
   void read();

   void loadConfig( const QString &grp );
   void saveConfig( const QString &grp);

  private:
   Mixer *m_mixer;
};

class KMixerWidget : public QWidget  {
   Q_OBJECT

  public:
   KMixerWidget( Mixer *mixer, bool small=false, bool vert=true, 
		 QWidget *parent=0, const char *name=0 );
   ~KMixerWidget();

   QString name() { return m_name; };
   void setName( QString name ) { m_name = name; };
   Mixer *mixer() { return m_mixer; };

  signals:
   void updateTicks( bool on );
   void updateLabels( bool on );
   void updateIcons( bool on );
   void updateLayout();

  public slots:     
   void setTicks( bool on );
   void setLabels( bool on );
   void setIcons( bool on );
   void setBalance( int value );
   // void setOrientation( int vert );

   void sessionSave( QString grp, bool sessionConfig );
   void sessionLoad( QString grp, bool sessionConfig );

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
   KActionCollection *m_actions;
   bool m_small;
   bool m_vertical;

   void mousePressEvent( QMouseEvent *e );
   void updateDevices( bool vert );
};

#endif
