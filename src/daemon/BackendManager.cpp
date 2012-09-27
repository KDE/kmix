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

#include "BackendManager.h"
#include "Control.h"
#include "ControlGroup.h"
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include "backends/PulseAudio.h"
#include "backends/ALSA.h"

BackendManager *BackendManager::s_instance = 0;

BackendManager::BackendManager()
{
    m_groups[Control::OutputStream] = new ControlGroup("Playback");
    m_groups[Control::InputStream] = new ControlGroup("Recording");
    m_groups[Control::HardwareInput] = new ControlGroup("Hardware Input");
    m_groups[Control::HardwareOutput] = new ControlGroup("Hardware Output");
    if (!addBackend(new Backends::PulseAudio(this))) {
        qWarning() << "PulseAudio is not working, falling back to ALSA.";
        addBackend(new Backends::ALSA(this));
    }
}

bool BackendManager::addBackend(Backend *backend)
{
    qDebug() << "Probing backend" << backend;
    if (backend->probe()) {
        connect(backend, SIGNAL(controlAdded(Control *)), this, SLOT(handleControlAdded(Control *)));
        connect(backend, SIGNAL(controlRemoved(Control *)), this, SLOT(handleControlRemoved(Control *)));
        m_backends << backend;
        qDebug() << "Loaded backend" << backend;
        return backend->open();
    }
    return false;
}

BackendManager *BackendManager::instance()
{
    if (s_instance == 0)
        s_instance = new BackendManager();
    return s_instance;
}

QList<ControlGroup*> BackendManager::groups() const
{
    return m_groups.values();
}

void BackendManager::handleControlAdded(Control *control)
{
    m_groups[control->category()]->addControl(control);
    emit controlAdded(control);
}

void BackendManager::handleControlRemoved(Control *control)
{
    m_groups[control->category()]->removeControl(control);
    emit controlRemoved(control);
}

#include "BackendManager.moc"
