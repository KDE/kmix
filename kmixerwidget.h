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
class MixSet;

class KMixerWidget : public QWidget  {
   Q_OBJECT

   friend class KMixerPrefWidget;

  public:
   KMixerWidget( MixSet *mixSet, QWidget *parent=0, const char *name=0 );
   ~KMixerWidget();

   QString mixerName();

  signals:
   void rightMouseClick();
   void updateTicks( bool on );

   public slots:
      virtual void applyPrefs( class KMixPrefWidget *prefWidget );
   virtual void initPrefs( class KMixPrefWidget *prefWidget );
   void setTicks( bool on );
   void setBalance( int value );

   void sessionSave( bool sessionConfig );
   void sessionLoad( bool sessionConfig );

   private slots:	
      void rightMouseClicked() { emit rightMouseClick(); };	

  protected:
   void updateSize();

  private:
   Mixer *m_mixer;
   MixSet *m_mixSet;
   QSlider *m_balanceSlider;
   QTimer *m_timer;
   QBoxLayout *m_topLayout;

   void mousePressEvent( QMouseEvent *e );
};

class KMixerPrefWidget : public QWidget {
   Q_OBJECT	

  public:
   KMixerPrefWidget( KMixerWidget* mixerWidget, QWidget *parent=0, const char *name=0 );
   ~KMixerPrefWidget();

  protected:
   KMixerWidget *m_mixerWidget;
   QBoxLayout *m_layout;
   QList<ChannelSetup> m_channels;
};

#endif
