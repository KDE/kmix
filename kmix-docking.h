/*
 *              kmix: KDE's small mixer
 *
 *              Copyright (C) 1997 Christian Esken
 *                       esken@kde.org
 * 
 * This file is based on a contribution by Harri Porten <porten@tu-harburg.de>
 *
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef KMIX_DOCKING_H
#define KMIX_DOCKING_H

#include "docking.h"


class KMixDockWidget : public KDockWidget {

  Q_OBJECT

public:
  KMixDockWidget(const QString& name=0, const QString& dockIconName=0);
  ~KMixDockWidget();
};

#endif

