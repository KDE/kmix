//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2004 Chrisitan Esken <esken@kde.org>
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

#ifndef MDWSWITCH_H
#define MDWSWITCH_H

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

class KLedButton;
class KAction;
class KActionCollection;
class KSmallSlider;
class KGlobalAccel;

class MixDevice;
class VerticalText;
class Mixer;
class ViewBase;

#include "mixdevicewidget.h"

class MDWSwitch : public MixDeviceWidget
{
    Q_OBJECT

public:
    MDWSwitch( Mixer *mixer, MixDevice* md,
	       bool small, Qt::Orientation orientation,
	       QWidget* parent = 0, ViewBase* mw = 0, const char* name = 0);
    ~MDWSwitch();

    void addActionToPopup( KAction *action );
    QSize sizeHint() const;
    void setBackgroundMode(BackgroundMode m);
    bool eventFilter( QObject* obj, QEvent* e );

public slots:
    // GUI hide and show
    void setDisabled();
    void setDisabled(bool);

    // Switch on/off
    void toggleSwitch();
    void setSwitch(bool value);

    void update();
    virtual void showContextMenu();

private:
    void createWidgets();

    QLabel        *_label;
    VerticalText  *_labelV;
    KLedButton    *_switchLED;
    QBoxLayout    *_layout;
};

#endif
