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

// KMix
class MixDevice;
class ViewBase;

// Qt
class QComboBox;
class QLabel;

#include "gui/mixdevicewidget.h"

class MDWEnum : public MixDeviceWidget
{
    Q_OBJECT

public:
    MDWEnum(shared_ptr<MixDevice> md, MixDeviceWidget::MDWFlags flags, ViewBase *view, ProfControl *pctl = nullptr);
    virtual ~MDWEnum() = default;

    QSizePolicy sizePolicy() const;
    int labelExtentHint() const override;
    void setLabelExtent(int extent) override;

public slots:
    // Enum handling: next and selecting
    void nextEnumId();
    int  enumId();
    void setEnumId(int value);

    void update() override;

protected:
    void createContextMenu(QMenu *menu) override;

private:
    void createWidgets();

    QLabel        *_label;
    QComboBox     *_enumCombo;
};

#endif
