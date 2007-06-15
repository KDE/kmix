//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
#ifndef ViewSwitches_h
#define ViewSwitches_h

class QLayout;
class QWidget;

class Mixer;
#include "viewbase.h"

class ViewSwitches : public ViewBase
{
    Q_OBJECT
public:
    ViewSwitches(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof);
    ~ViewSwitches();

    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();
    virtual void configurationUpdate();

    QSize sizeHint() const;

public slots:
    virtual void refreshVolumeLevels();

private:
    QLayout* _layoutMDW;
    QLayout* _layoutEnum;
    QLayout* _layoutSwitch;
};

#endif

