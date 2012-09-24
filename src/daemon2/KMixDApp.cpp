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
{
    // One way or another we need to create this to show up on DBus
    BackendManager::instance();
}

KMixDApp::~KMixDApp()
{
}

int KMixDApp::start()
{
    if (QDBusConnection::sessionBus().registerService("org.kde.kmixd")) {
        new KMixDAdaptor(this);
        QDBusConnection::sessionBus().registerObject("/KMixD", this);
        return exec();
    }
    return 1;
}

void KMixDApp::setMaster(const QString &masterID)
{
    Q_ASSERT(false);
}

QStringList KMixDApp::mixerGroups() const
{
    QStringList ret;
    foreach(ControlGroup *group, BackendManager::instance()->groups()) {
        ret << QString("/groups/%1").arg(group->id());
    }
    qDebug() << ret;
    return ret;
}

QString KMixDApp::masterControl() const
{
    Q_ASSERT(false);
    return QString();
}

int KMixDApp::masterVolume() const
{
    Q_ASSERT(false);
    return 0;
}

void KMixDApp::setMasterVolume(int v)
{
    Q_ASSERT(false);
}
