/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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


//KMix
#include "mdwmoveaction.h"
#include "core/mixdevice.h"

// Qt
#include <QString>

MDWMoveAction::MDWMoveAction(shared_ptr<MixDevice> md, QObject *parent)
 : KAction(parent), m_mixDevice(md)
{
   Q_ASSERT(md);

   setText(m_mixDevice->readableName());
   setIcon(KIcon(m_mixDevice->iconName()));
   connect(this, SIGNAL(triggered(bool)), SLOT(triggered(bool)));
}

MDWMoveAction::~MDWMoveAction()
{
}

void MDWMoveAction::triggered(bool checked)
{
    Q_UNUSED(checked);
    emit moveRequest(m_mixDevice->id());
}
