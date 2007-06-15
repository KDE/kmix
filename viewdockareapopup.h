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
#ifndef ViewDockAreaPopup_h
#define ViewDockAreaPopup_h

#include "viewbase.h"

class QMouseEvent;
class QGridLayout;
class QWidget;
class QPushButton;

class Mixer;
class KMixDockWidget;
class MixDeviceWidget;
class MixDevice;
class QFrame;
class QTime;

class ViewDockAreaPopup : public ViewBase
{
    Q_OBJECT
public:
    ViewDockAreaPopup(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof, KMixDockWidget *dockW);
    ~ViewDockAreaPopup();
    MixDevice* dockDevice();

    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();
    virtual void refreshVolumeLevels();
    virtual void showContextMenu();

    QSize sizeHint() const;
    bool justHidden();

protected:
    MixDeviceWidget *_mdw;
    KMixDockWidget  *_dock;
    MixDevice       *_dockDevice;
    QPushButton     *_showPanelBox;

    void mousePressEvent(QMouseEvent *e);
    void wheelEvent ( QWheelEvent * e );

private:
    QGridLayout* _layoutMDW;
    QFrame *_frame;
    QTime *_hideTimer;

private slots:
	 void showPanelSlot();

};

#endif

