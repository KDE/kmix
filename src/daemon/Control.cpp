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

#include "controlmonitor_interface.h"

QAtomicInt Control::s_id = 0;

Control::Control(Category category, QObject *parent)
    : QObject(parent)
    , m_category(category)
    , m_currentTarget(0)
{
    new ControlAdaptor(this);
    m_id = s_id.fetchAndAddRelaxed(1);
    QDBusConnection::sessionBus().registerObject(QString("/controls/%1").arg(m_id), this);
    m_monitorMap = new QSignalMapper(this);
}

int Control::id() const
{
    return m_id;
}

Control::Category Control::category() const
{
    return m_category;
}

Control::~Control()
{
    emit removed();
}

void Control::subscribeMonitor(const QString &address, const QString &path)
{
    org::kde::KMix::ControlMonitor *monitor = new org::kde::KMix::ControlMonitor(address, path, QDBusConnection::sessionBus());
    int pos = m_monitors.size();
    m_monitors << monitor;
    m_monitorMap->setMapping(monitor, pos);
    connect(monitor, SIGNAL(removed()), m_monitorMap, SLOT(map()));
    startMonitor();
}

void Control::removeMonitor(int pos)
{
    m_monitors.removeOne(m_monitors[pos]);
    if (m_monitors.size() == 0) {
        stopMonitor();
    }
}

void Control::levelUpdate(int l)
{
    foreach(org::kde::KMix::ControlMonitor *monitor, m_monitors) {
        monitor->levelUpdate(l);
    }
}

bool Control::canChangeTarget() const
{
    qDebug() << m_targets.size() << "targets";
    return m_targets.size() > 1;
}

void Control::addAlternateTarget(Control *target)
{
    m_targets << target;
    qDebug() << "Added alternate target" << m_targets;
}

void Control::removeAlternateTarget(Control *target)
{
    m_targets.removeOne(target);
}

QStringList Control::alternateTargets() const
{
    QStringList ret;
    foreach(Control *c, m_targets) {
        ret << QString("/controls/%1").arg(c->id());
    }
    return ret;
}

QString Control::currentTarget() const
{
    if (m_currentTarget)
        return QString("/controls/%1").arg(m_currentTarget->id());
    return QString();
}

void Control::setCurrentTarget(Control *t)
{
    m_currentTarget = t;
    emit currentTargetChanged(QString("/controls/%1").arg(t->id()));
}

void Control::changeTarget(Control *t)
{
    Q_UNUSED(t);
}

void Control::startMonitor()
{
}

void Control::stopMonitor()
{
}

bool Control::canMonitor() const
{
    return false;
}

void Control::setTarget(int idx)
{
    foreach(Control *target, m_targets) {
        if (target->id() == idx) {
            changeTarget(target);
            return;
        }
    }
}

#include "Control.moc"
