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

#include "KMixDApp.h"
#include "kmixdadaptor.h"
#include "ControlGroup.h"
#include "BackendManager.h"
#include "Control.h"

#include <QtDBus/QDBusConnection>
#include <QtCore/QDebug>

KMixDApp::KMixDApp(int &argc, char **argv)
    : QCoreApplication(argc, argv)
    , m_master(0)
{
    connect(BackendManager::instance(), SIGNAL(controlAdded(Control*)), this, SLOT(controlAdded(Control*)));
    connect(BackendManager::instance(), SIGNAL(controlRemoved(Control*)), this, SLOT(controlRemoved(Control*)));
}

KMixDApp::~KMixDApp()
{
}

int KMixDApp::start()
{
    new KMixDAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/KMixD", this);
    if (QDBusConnection::sessionBus().registerService("org.kde.kmixd")) {
        return exec();
    }
    return 1;
}

void KMixDApp::setMaster(int id)
{
    Q_ASSERT(false);
}

QStringList KMixDApp::mixerGroups() const
{
    QStringList ret;
    foreach(ControlGroup *group, BackendManager::instance()->groups()) {
        ret << QString("/groups/%1").arg(group->id());
    }
    return ret;
}

QString KMixDApp::masterControl() const
{
    return QString("/controls/%1").arg(m_master->id());
}

int KMixDApp::masterVolume() const
{
    if (m_master) {
        int sum = 0;
        for(int i = 0;i<m_master->channels();i++) {
            sum+=m_master->getVolume(i);
        }
        return sum/m_master->channels();
    }
    return 0;
}

void KMixDApp::setMasterVolume(int v)
{
    if (m_master) {
        for(int i = 0;i<m_master->channels();i++) {
            qDebug() << "Setting channel" << i << "on master to" << v;
            m_master->setVolume(i, v);
        }
    }
}

void KMixDApp::controlAdded(Control *control)
{
    if (control->category() == Control::HardwareOutput) {
        if (m_master)
            disconnect(m_master, SIGNAL(volumeChanged(int, int)), this, SLOT(updateMasterVolume()));
        m_master = control;
        emit masterChanged(QString("/controls/%1").arg(m_master->id()));
        qDebug() << "Setting master to" << m_master << m_master->displayName();
        connect(m_master, SIGNAL(volumeChanged(int, int)), this, SLOT(updateMasterVolume()));
        connect(m_master, SIGNAL(muteChanged(bool)), this, SLOT(updateMasterMute()));
    }
}

void KMixDApp::updateMasterVolume()
{
    qDebug() << "Master volume updated";
    emit masterVolumeChanged(masterVolume());
}

void KMixDApp::updateMasterMute()
{
    qDebug() << "Master mute updated";
    emit masterMuteChanged(masterMute());
}

void KMixDApp::controlRemoved(Control *control)
{
    if (control == m_master) {
        m_master = 0;
    }
}

void KMixDApp::setMasterMute(bool muted)
{
    if (m_master) {
        m_master->setMute(muted);
    }
}

bool KMixDApp::masterMute() const
{
    if (m_master) {
        return m_master->isMuted();
    }
    return true;
}

void KMixDApp::increaseMaster()
{
    qDebug() << "Bumping master from" << masterVolume();
    setMasterVolume(masterVolume()+USHRT_MAX/10);
}

void KMixDApp::decreaseMaster()
{
    setMasterVolume(masterVolume()-USHRT_MAX/10);
}
