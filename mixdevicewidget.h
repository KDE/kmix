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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef MIXDEVICEWIDGET_H
#define MIXDEVICEWIDGET_H

#include <kpanelapplet.h>

#include <qwidget.h>
#include "volume.h"
#include <qptrlist.h>
#include <qpixmap.h>
#include <qrangecontrol.h>

class QBoxLayout;
class QLabel;
class QPopupMenu;
class QSlider;

class KLed;
class KLedButton;
class KAction;
class KActionCollection;
class KSmallSlider;
class KGlobalAccel;

class MixDevice;
class VerticalText;
class Mixer;
class ViewBase;

class MixDeviceWidget
 : public QWidget
{
      Q_OBJECT

public:
    enum ValueStyle { NNONE = 0, NABSOLUTE = 1, NRELATIVE = 2 } ;

      MixDeviceWidget( Mixer *mixer, MixDevice* md,
                       bool small, Qt::Orientation orientation,
                       QWidget* parent = 0, ViewBase* mw = 0, const char* name = 0);
    ~MixDeviceWidget();

    void addActionToPopup( KAction *action );

    virtual bool isDisabled() const;
    MixDevice* mixDevice() { return m_mixdevice; };

    virtual void setColors( QColor high, QColor low, QColor back );
    virtual void setIcons( bool value );
    virtual void setMutedColors( QColor high, QColor low, QColor back );

    virtual bool isStereoLinked() const { return false; };
    //virtual bool isLabeled() const { return false; };
    virtual void setStereoLinked( bool ) {};
    virtual void setLabeled( bool );
    virtual void setTicks( bool ) {};
    virtual void setValueStyle( ValueStyle ) {};

    virtual KGlobalAccel *keys(void);

public slots:
    virtual void setDisabled( bool value );
    virtual void defineKeys();
    virtual void update();
    virtual void showContextMenu();

signals:
    void newVolume( int num, Volume volume );
    void newMasterVolume( Volume volume );
    void masterMuted( bool );
    void newRecsrc( int num, bool on );

protected slots:
    void volumeChange( int );
    virtual void setVolume( int channel, int volume );
    virtual void setVolume( Volume volume );

protected:
      Mixer*               m_mixer;
      MixDevice*           m_mixdevice;
      KActionCollection*   _mdwActions;
      KGlobalAccel*        m_keys;
      ViewBase*            m_mixerwidget;
      bool                 m_disabled;
      Qt::Orientation      _orientation;
      bool                 m_small;

private:
      void mousePressEvent( QMouseEvent *e );
};

#endif
