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
#include "ControlGroup.h"
#include "Control.h"
#include <QtCore/QStringList>
#include <QtDBus/QDBusConnection>

QAtomicInt ControlGroup::s_id = 0;

ControlGroup::ControlGroup(const QString &displayName, QObject *parent)
    : QObject(parent)
    , m_displayName(displayName)
{
    QDBusConnection::sessionBus().registerObject(QString("/groups/%1").arg(s_id), this);
    s_id.ref();
}

ControlGroup::~ControlGroup()
{
    s_id.deref();
}

QString ControlGroup::displayName() const
{
    return m_displayName;
}

QStringList ControlGroup::controls() const
{
    return QStringList();
}

void ControlGroup::addControl(Control *control)
{
    m_controls[control->displayName()] = control;
    emit controlAdded(control->displayName());
}
