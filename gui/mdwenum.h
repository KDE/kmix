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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MDWENUM_H
#define MDWENUM_H

#include <QWidget>
#include "core/volume.h"

// KMix
class MixDevice;
class ViewBase;

// KDE
class KAction;
class KComboBox;

// Qt
class QBoxLayout;
class QLabel;

#include "gui/mixdevicewidget.h"

class MDWEnum : public MixDeviceWidget
{
    Q_OBJECT

public:
    MDWEnum( shared_ptr<MixDevice> md,
	       Qt::Orientation orientation,
	       QWidget* parent, ViewBase* view, ProfControl* pctl);
    ~MDWEnum();

    void addActionToPopup( KAction *action );
    QSizePolicy sizePolicy() const;
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
    virtual void showContextMenu(const QPoint& pos = QCursor::pos());

private:
    void createWidgets();

    QLabel        *_label;
    KComboBox     *_enumCombo;
    QBoxLayout    *_layout;
};

#endif
