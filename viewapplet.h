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
#ifndef ViewApplet_h
#define ViewApplet_h

#include "viewbase.h"
#include <plasma/kpanelapplet.h>     // ??? plasma/

class QBoxLayout;
class Mixer;

class ViewApplet : public ViewBase
{
    Q_OBJECT
public:
    ViewApplet(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, GUIProfile *guiprof, Plasma::Position position);
    ~ViewApplet();

    virtual void setMixSet();
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();
    virtual void configurationUpdate();

    QSize       sizeHint() const;
    QSizePolicy sizePolicy() const;
    virtual void resizeEvent(QResizeEvent*);

signals:
    void appletContentChanged();

public slots:
   virtual void refreshVolumeLevels();

private:
    QBoxLayout*   _layoutMDW;
    // Position of the applet (pLeft, pRight, pTop, pBottom)
    //KPanelApplet::Position  _KMIXposition;
    // Orientation of the applet (horizontal or vertical)
    Qt::Orientation _viewOrientation;
};

#endif

