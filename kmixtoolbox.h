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
 * Software Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef KMIXTOOLBOX_H
#define KMIXTOOLBOX_H

#include "qlist.h"
#include "qwidget.h"

class ViewBase;

class KConfig;

/**
 * This toolbox contains various static methods that are shared throughout KMix.
 * The reason, why it is not put in a common base class is, that the classes are
 * very different and cannot be changed (e.g. KPanelApplet) without major headache.
 */

class KMixToolBox {
 public:
    static void setIcons  (QList<QWidget *> &mdws, bool on );
    static void setLabels (QList<QWidget *> &mdws, bool on );
    static void setTicks  (QList<QWidget *> &mdws, bool on );
    static void loadView(ViewBase *view, KConfig *config);
    static void saveView(ViewBase *view, KConfig *config);
    static void loadKeys(ViewBase *view, KConfig *config);
    static void saveKeys(ViewBase *view, KConfig *config);
};
    

#endif
