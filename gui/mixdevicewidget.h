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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MIXDEVICEWIDGET_H
#define MIXDEVICEWIDGET_H

#include <QWidget>
#include "core/mixdevice.h"
#include "core/volume.h"
#include <qpixmap.h>


class KAction;
class KActionCollection;
class KShortcutsDialog;

class MixDevice;
class ProfControl;
class ViewBase;

class MixDeviceWidget
 : public QWidget
{
      Q_OBJECT

public:
    MixDeviceWidget( shared_ptr<MixDevice> md,
                     bool small, Qt::Orientation orientation,
                     QWidget* parent, ViewBase*, ProfControl * );
    virtual ~MixDeviceWidget();

    void addActionToPopup( KAction *action );

    virtual bool isDisabled() const;
    shared_ptr<MixDevice> mixDevice() { return m_mixdevice; }

    virtual void setColors( QColor high, QColor low, QColor back );
    virtual void setIcons( bool value );
    virtual void setMutedColors( QColor high, QColor low, QColor back );

    virtual bool isStereoLinked() const { return false; }
    virtual void setStereoLinked( bool ) {}
    virtual void setLabeled( bool );
    virtual void setTicks( bool ) {}


public slots:
    virtual void setDisabled( bool value );
    virtual void defineKeys();
    virtual void update();
    virtual void showContextMenu( const QPoint &pos = QCursor::pos() );

protected slots:
    void volumeChange( int );
//    virtual void setVolume( int channel, int volume );
//    virtual void setVolume( Volume volume );

protected:

      shared_ptr<MixDevice>  m_mixdevice;
      KActionCollection*   _mdwActions;
      KActionCollection*   _mdwPopupActions;
      ViewBase*            m_view;
      ProfControl*         _pctl;
      bool                 m_disabled;
      Qt::Orientation      _orientation;
      bool                 m_small;
      KShortcutsDialog*    m_shortcutsDialog;

private:
      void mousePressEvent( QMouseEvent *e );
};

#endif
