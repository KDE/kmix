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

#include "ControlMonitor.h"
#include "control_interface.h"
#include "controlmonitoradaptor.h"
#include <QtGui/QProgressBar>

QAtomicInt ControlMonitor::s_id = 0;

ControlMonitor::ControlMonitor(QProgressBar *widget, org::kde::KMix::Control *control, QObject *parent)
    : QObject(parent)
    , m_widget(widget)
{
    new ControlMonitorAdaptor(this);
    QString path = QString("/org/kde/kmix/monitors/%1").arg(s_id.ref());
    QDBusConnection::sessionBus().registerObject(path, this);
    control->subscribeMonitor(QDBusConnection::sessionBus().baseService(), path);
}

ControlMonitor::~ControlMonitor()
{
    emit removed();
}

void ControlMonitor::levelUpdate(int level)
{
    qDebug() << "Level update to" << level;
    m_widget->setValue(level);
}

#include "ControlMonitor.moc"
