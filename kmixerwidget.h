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
class QTimer;
class Channel;

class KMixerWidget : public QWidget  {
   Q_OBJECT

  public:
   KMixerWidget( Mixer *mixer, QWidget *parent=0, const char *name=0 );
   ~KMixerWidget();

   QString name() { return m_name; };
   void setName( QString name ) { m_name = name; };
   Mixer *mixer() { return m_mixer; };

  signals:
   void updateTicks( bool on );

  public slots:     
   void setTicks( bool on );
   void setBalance( int value );

   void sessionSave( QString grp, bool sessionConfig );
   void sessionLoad( QString grp, bool sessionConfig );

  private slots:	
   void rightMouseClicked();	

  protected:
   void updateSize();

  private:
   Mixer *m_mixer;
   QSlider *m_balanceSlider;
   QTimer *m_timer;
   QBoxLayout *m_topLayout;
   QList<Channel> m_channels;
   QString m_name;

   void mousePressEvent( QMouseEvent *e );
};

#endif
