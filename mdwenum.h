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

#ifndef MDWENUM_H
#define MDWENUM_H

#include <qwidget.h>
#include "volume.h"

class QBoxLayout;

class KAction;
class KActionCollection;
class KComboBox;
class KGlobalAccel;

class MixDevice;
class Mixer;
class ViewBase;

#include "mixdevicewidget.h"

class MDWEnum : public MixDeviceWidget
{
    Q_OBJECT

public:
    MDWEnum( Mixer *mixer, MixDevice* md,
	       Qt::Orientation orientation,
	       QWidget* parent = 0, ViewBase* mw = 0, const char* name = 0);
    ~MDWEnum();

    void addActionToPopup( KAction *action );
    QSize sizeHint() const;
    bool eventFilter( QObject* obj, QEvent* e );

public slots:
    // GUI hide and show
    void setDisabled();
    void setDisabled(bool);

    // Enum handling: next and selecting
    void nextEnumId();
    int  enumId();
    void setEnumId(int value);

    void update();
    virtual void showContextMenu();

private:
    void createWidgets();

    QLabel        *_label;
    KComboBox     *_enumCombo;
    QBoxLayout    *_layout;
};

#endif
