/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-98 Christian Esken
 *                        esken@kde.org
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

#include "kmix.h"

#include <qtooltip.h>
#include <kwm.h>
#include <kapp.h>

#include "kmix-docking.h"


KMixDockWidget::KMixDockWidget(const char *name, 
			       const QString& dockIconName) 
    : KDockWidget(name, dockIconName)
{
}

KMixDockWidget::~KMixDockWidget()
{
}



#include "kmix-docking.moc"

