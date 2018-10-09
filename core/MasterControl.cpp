/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2011 Christian Esken <esken@kde.org>
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

#include "MasterControl.h"

MasterControl::MasterControl()
{
}

MasterControl::~MasterControl()
{
}

QString MasterControl::getCard() const
{
    return card;
}

QString MasterControl::getControl() const
{
    return control;
}

void MasterControl::set(QString card, QString control)
{
    this->card = card;
    this->control = control;
}

bool MasterControl::isValid()
{
    if ( control.isEmpty() || card.isEmpty() )
        return false;

    return true;
}

