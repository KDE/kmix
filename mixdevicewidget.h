//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 *               1996-2000 Christian Esken <esken@kde.org>
 *                         Sven Fischer <herpes@kawo2.rwth-aachen.de>
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

#include <kpanelapplet.h>

#include <qwidget.h>
#include <volume.h>
#include <qlist.h>
#include <qpixmap.h>
#include <qrangecontrol.h>

class KLed;
class QLabel;
class QPopupMenu;
class KLedButton;
class MixDevice;
class KActionCollection;
class Mixer;
class QTimer;
class KSmallSlider;
class QSlider;
class KGlobalAccel;

class MixDeviceWidget
 : public QWidget
{
      Q_OBJECT

   public:
      MixDeviceWidget( Mixer *mixer, MixDevice* md,
                       bool showMuteLED, bool showRecordLED,
                       bool small, KPanelApplet::Direction dir,
                       QWidget* parent = 0, const char* name = 0);
      ~MixDeviceWidget();

      bool isDisabled();
      bool isMuted();
      bool isRecsrc();
      bool isStereoLinked() { return m_linked; };
      bool isLabeled();

      void setStereoLinked( bool value );
      void setLabeled( bool value );
      void setTicks( bool ticks );
      void setIcons( bool value );
      void setColors( QColor high, QColor low, QColor back );
      void setMutedColors( QColor high, QColor low, QColor back );

      KGlobalAccel *keys(void);

   public slots:
      void toggleRecsrc();
      void toggleMuted();
      void toggleStereoLinked();

      void setDisabled() { setDisabled( true ); };
      void setDisabled( bool value );

      void defineKeys();
   signals:
      void newVolume( int num, Volume volume );
      void newRecsrc( int num, bool on );
      void updateLayout();
      void rightMouseClick();

   private slots:
      void setRecsrc( bool value );
      void setMuted( bool value );
      void setUnmuted( bool value) { setMuted( !value ); };
      void setVolume( int channel, int volume );
      void setVolume( Volume volume );
      void contextMenu();
      void update();
      void volumeChange( int );

      void increaseVolume();
      void decreaseVolume();
   private:
      QPixmap getIcon( int icon );
      void setIcon( int icontype );

      void mousePressEvent( QMouseEvent *e );
      bool eventFilter( QObject*, QEvent* );

      void createWidgets( bool showMuteLED, bool showRecordLED );

      Mixer *m_mixer;
      MixDevice *m_mixdevice;
      QTimer *m_updateTimer;
      QList<QWidget> m_sliders;
      KActionCollection *m_actions;
      KGlobalAccel *m_keys;

      bool m_linked;
      bool m_disabled;
      KPanelApplet::Direction m_direction;
      bool m_small;

      QLabel *m_iconLabel;
      KLedButton *m_muteLED;
      KLedButton *m_recordLED;
      QLabel *m_label;

};

#endif
