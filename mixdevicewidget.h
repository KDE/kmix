//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 *		 1996-2000 Christian Esken <esken@kde.org>
 *        		   Sven Fischer <herpes@kawo2.rwth-aachen.de>
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

#ifndef MIXDEVICEWIDGET_H
#define MIXDEVICEWIDGET_H

#include <qwidget.h>
#include <volume.h>
#include <qlist.h>

class KLed;
class QSlider;
class QLabel;
class QPopupMenu;
class KLedButton;
class MixDevice;
class KActionCollection;

class MixDeviceWidget : public QWidget
{
      Q_OBJECT

   public:

      MixDeviceWidget( MixDevice* md, bool showMuteLED, bool showRecordLED,
		       QWidget* parent = 0, const char* name = 0);
      ~MixDeviceWidget();

      MixDevice* mixDevice() const { return m_mixdevice; };    
      
      bool isDisabled();
      bool isMuted();
      bool isUnmuted() { return !isMuted(); };
      bool isRecsrc();
      bool isStereoLinked();
      bool isLabeled();
      
   public slots:
      void setRecsrc( bool value );
      void setDisabled() { setDisabled( true ); };
      void setDisabled( bool value );
      void setMuted( bool value );
      void setUnmuted( bool value) { setMuted( !value ); };
      void setStereoLinked( bool value );
      void setLabeled( bool value );
      void setTicks( bool ticks );

      void toggleRecsrc();
      void toggleMuted();
      void toggleStereoLinked();

      void setVolume( int channel, int volume );
      void setVolume( Volume volume );

      void setIcon( int icontype );

      void updateSliders();
      void updateRecsrc();

   private slots:
      void volumeChange( int );
      void contextMenu();

   signals:
      void newVolume( int num, Volume volume );
      void newRecsrc( int num, bool on );
      void rightMouseClick();

   private:
      MixDevice *m_mixdevice;
      QList<QSlider> m_sliders;
      bool m_split;
      bool m_show;

      QLabel *m_iconLabel;
      KLedButton *m_muteLED;
      KLedButton *m_recordLED;
      QPopupMenu *m_popupMenu;
      QLabel *m_label;
      KActionCollection *m_actions;

      void mousePressEvent( QMouseEvent *e );
      bool eventFilter( QObject*, QEvent* );
};


#endif
