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
#ifndef ViewSurround_h
#define ViewSurround_h

class QBoxLayout;
class QGridLayout;
class QWidget;

class MixDevice;
class MixDeviceWidget;
class Mixer;
#include "viewbase.h"

class ViewSurround : public ViewBase
{
    Q_OBJECT
public:
    ViewSurround(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof);
    ~ViewSurround();

    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();

    QSize sizeHint() const;

public slots:
    virtual void refreshVolumeLevels();

private:
    MixDeviceWidget* createMDW(MixDevice *md, bool small, Qt::Orientation orientation);
    MixDevice *_mdSurroundFront;
    MixDevice *_mdSurroundBack;

    QBoxLayout* _layoutMDW;
    QBoxLayout* _layoutSliders;
    QGridLayout* _layoutSurround;
};

#endif

