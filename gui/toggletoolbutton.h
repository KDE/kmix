//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2018 Jonathan Marten <jjm@keelhaul.me.uk>
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
 * License along with this program; if not, see
 * <https://www.gnu.org/licenses>.
 */

#ifndef TOGGLETOOLBUTTON_H
#define TOGGLETOOLBUTTON_H


#include <qtoolbutton.h>
#include <qpixmap.h>


/**
 * A tool button that can maintain two pixmaps, for an active and inactive state.
 *
 * @see QToolButton
 **/

class ToggleToolButton : public QToolButton
{
    Q_OBJECT

public:
    explicit ToggleToolButton(const QString &activeIconName, QWidget *pnt = nullptr);
    virtual ~ToggleToolButton() = default;

    void setActive(bool active = true);
    void setSmallSize(bool small = true)		{ mSmallSize = small; }
    bool isActive() const				{ return (mIsActive); }

    void setActiveIcon(const QString &name)		{ mActiveIconName = name; }
    void setInactiveIcon(const QString &name)		{ mInactiveIconName = name; }

    static void setIndicatorIcon(const QString &iconName, QWidget *label, bool small = false);

private:
    bool mSmallSize;
    QString mActiveIconName;
    QString mInactiveIconName;

    bool mIsActive;
    bool mFirstTime;

    QIcon mActiveIcon;
    bool mActiveLoaded;
    QIcon mInactiveIcon;
    bool mInactiveLoaded;
};

#endif							// TOGGLETOOLBUTTON_H
