//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 *							 1996-2000 Christian Esken <esken@kde.org>
 *        								 Sven Fischer <herpes@kawo2.rwth-aachen.de>
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

#ifndef MIXDEVICE_H
#define MIXDEVICE_H

#include <qwidget.h>
#include <volume.h>
#include <qlist.h>

class KLed;
class QSlider;
class QLabel;
class QPopupMenu;

/*
  I am using a fixed MAX_MIXDEVS #define here.
   People might argue, that I should rather use the SOUND_MIXER_NRDEVICES
   #define used by OSS. But using this #define is not good, because it is
   evaluated during compile time. Compiling on one platform and running
   on another with another version of OSS with a different value of
   SOUND_MIXER_NRDEVICES is very bad. Because of this, usage of
   SOUND_MIXER_NRDEVICES should be discouraged.

   The #define below is only there for internal reasons.
   In other words: Don't play around with this value
 */
#define MAX_MIXDEVS 32

class MixDevice
{
public:

	// For each ChannelType a special icon exists
  enum ChannelType {AUDIO = 1, BASS, CD, EXTERNAL, MICROPHONE,
                    MIDI, RECMONITOR, TREBLE, UNKNOWN, VOLUME };

  MixDevice(int num, Volume vol, bool recordable,
            QString name, ChannelType type = UNKNOWN );
  ~MixDevice() {};

  int num() const   	         { return m_num; };
  QString	name() const         { return m_name; };
  bool isStereo() const        { return (m_volume.channels() > 1); };
  bool isRecordable() const    { return m_recordable; };
  bool isRecsrc() const        { return m_recsrc; };
  bool isMuted() const         { return m_volume.isMuted(); };
  bool isStereoLinked() const  { return m_stereoLink; };
  bool isDisabled() const      { return m_disabled; };

  void setDisabled(bool value)         { m_disabled = value; };
  void setMuted(bool value)            { m_volume.setMuted( value ); };
  void setStereoLinked(bool value)     { m_stereoLink = value; };
  void setRecsrc(bool value)           { m_recsrc = value; };

  void setVolume( Volume volume ) { m_volume = volume; };
  void setVolume( int channel, int volume );
  int getVolume( int channel ) const;
  Volume getVolume() const { return m_volume; };
  int rightVolume() const;
  int leftVolume() const;

  void read(int set);
  void write(int set);

  void setType( ChannelType channeltype ) { m_type = channeltype; };
  ChannelType type() { return m_type; };

protected:
  Volume m_volume;
  ChannelType m_type;
  int	m_num; // ioctl() device number of mixer
  bool m_stereoLink; // Is this channel linked via the
                     // left-right-controller?
  bool m_recordable; // Can it be recorded?
  bool m_disabled; // Is it disabled?
  bool m_recsrc; // Is it an actual record source?
  QString	m_name;	// Ascii channel name
};


class KLedButton;

class MixDeviceWidget : public QWidget
{
  Q_OBJECT

public:

  MixDeviceWidget( MixDevice* md, bool showMuteLED, bool showRecordLED,
									 QWidget* parent = 0, const char* name = 0);
  ~MixDeviceWidget();

  MixDevice* mixDevice() const { return m_mixdevice; };

  bool eventFilter( QObject*, QEvent* );

public slots:
  void setRecsrc(bool value);
  void setDisabled(bool value);
  void setMuted(bool value);
	void setUnmuted(bool value) { setMuted( !value ); };
  void setStereoLinked(bool value);

  void toggleRecsrc();
  void toggleMuted();
  void toggleStereoLinked();

  void setVolume( int channel, int volume );
  void setVolume( Volume volume );

  void setIcon( int icontype );

  void updateSliders();
  void updateTicks( bool );
  void updateRecsrc();

private slots:
  void volumeChange( int );

signals:
  void newVolume( int num, Volume volume );
  void newRecsrc( int num, bool on );
	void rightMouseClick();

private:
  MixDevice *m_mixdevice;
  QList<QSlider> m_sliders;

  QLabel *m_iconLabel;
  KLedButton *m_muteLED;
  KLedButton *m_recordLED;
  QPopupMenu *m_popupMenu;

	void mousePressEvent( QMouseEvent *e );
};


#endif
