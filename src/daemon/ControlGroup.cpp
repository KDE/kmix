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
#include "controlgroupadaptor.h"
#include "Control.h"
#include <QtCore/QStringList>
#include <QtDBus/QDBusConnection>

QAtomicInt ControlGroup::s_id = 0;

ControlGroup::ControlGroup(const QString &displayName, QObject *parent)
    : QObject(parent)
    , m_displayName(displayName)
{
    new ControlGroupAdaptor(this);
    m_id = s_id.fetchAndAddRelaxed(1);
    QDBusConnection::sessionBus().registerObject(QString("/groups/%1").arg(m_id), this);
}

int ControlGroup::id() const
{
    return m_id;
}

ControlGroup::~ControlGroup()
{
}

QString ControlGroup::displayName() const
{
    return m_displayName;
}

QStringList ControlGroup::controls() const
{
    QStringList ret;
    foreach(Control *control, m_controls) {
        ret << QString("/controls/%1").arg(control->id());
    }
    return ret;
}

void ControlGroup::removeControl(Control *control)
{
    m_controls.take(control->displayName());
    emit controlRemoved(QString("/controls/%1").arg(control->id()));
}

void ControlGroup::addControl(Control *control)
{
    m_controls[control->displayName()] = control;
    emit controlAdded(QString("/controls/%1").arg(control->id()));
}

Control *ControlGroup::getControl(const QString &name) const
{
    return m_controls[name];
}
