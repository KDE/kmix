/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) Trever Fischer <tdfischer@fedoraproject.org>
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
#include "Control.h"
#include "controladaptor.h"
#include <QtDBus/QDBusConnection>

QAtomicInt Control::s_id = 0;

Control::Control(QObject *parent)
    : QObject(parent)
{
    new ControlAdaptor(this);
    m_id = s_id.fetchAndAddRelaxed(1);
    QDBusConnection::sessionBus().registerObject(QString("/controls/%1").arg(m_id), this);
}

int Control::id() const
{
    return m_id;
}

Control::~Control()
{
}

#include "Control.moc"
