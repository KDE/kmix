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

class MixDeviceWidget : public QWidget
{
      Q_OBJECT

   public:
      MixDeviceWidget( Mixer *mixer, MixDevice* md,
		       QWidget* parent = 0, const char* name = 0);
      ~MixDeviceWidget();
      
      bool isDisabled();
      bool isMuted();
      bool isUnmuted() { return !isMuted(); };
      bool isRecsrc();
      bool isStereoLinked();
      
   public slots:
      void setRecsrc( bool value );
      void setDisabled() { setDisabled( true ); };
      void setDisabled( bool value );
      void setMuted( bool value );
      void setUnmuted( bool value) { setMuted( !value ); };
      void setVolume( int channel, int volume );
      void setVolume( Volume volume );
      
      virtual void setStereoLinked( bool value );
      
      void toggleRecsrc();
      void toggleMuted();
      void toggleStereoLinked();

   signals:
      void newVolume( int num, Volume volume );
      void newRecsrc( int num, bool on );
      void updateLayout();

   protected:
      Mixer *m_mixer;
      MixDevice *m_mixdevice;
      KActionCollection *m_actions;

      QPixmap getIcon( int icon );

   private slots:
      virtual void update();

   private:
      bool m_linked;
      bool m_show;
      QTimer *m_timer;
};

class BigMixDeviceWidget : public MixDeviceWidget
{
      Q_OBJECT

   public:
      BigMixDeviceWidget( Mixer *mixer, MixDevice* md, bool showMuteLED, 
			  bool showRecordLED, bool vert,
			  QWidget* parent = 0, const char* name = 0);
      ~BigMixDeviceWidget();   

      bool isLabeled();
         
   public slots:
      void setStereoLinked( bool value );     
      void setIcon( int icontype );
      void setLabeled( bool value );
      void setTicks( bool ticks );
      void setIcons( bool value );

   private slots:
      void volumeChange( int );
      virtual void update();
      void contextMenu();

   signals:
      void rightMouseClick();

   private:
      QList<QSlider> m_sliders;

      QLabel *m_iconLabel;
      KLedButton *m_muteLED;
      KLedButton *m_recordLED;
      QPopupMenu *m_popupMenu;
      QLabel *m_label;     

      void mousePressEvent( QMouseEvent *e );
      bool eventFilter( QObject*, QEvent* );
};

class SmallMixDeviceWidget : public MixDeviceWidget
{
      Q_OBJECT

   public:
      SmallMixDeviceWidget( Mixer *mixer, MixDevice* md, bool vert,
			    QWidget* parent = 0, const char* name = 0);
      ~SmallMixDeviceWidget();   
         
   public slots:
      void setStereoLinked( bool value );     
      void setIcon( int icontype );
      void setIcons( bool value );

   private slots:
      void volumeChange( int );
      virtual void update();
      void contextMenu();

   signals:
      void rightMouseClick();

   private:
      QList<KSmallSlider> m_sliders;

      QLabel *m_iconLabel;
      QPopupMenu *m_popupMenu;

      void mousePressEvent( QMouseEvent *e );
      bool eventFilter( QObject*, QEvent* );
};

#endif
