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
#ifndef BACKENDMANAGER_H
#define BACKENDMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include "Control.h"

class ControlGroup;
class Backend;

class BackendManager : public QObject {
    Q_OBJECT
public:
    static BackendManager *instance();
    QList<ControlGroup*> groups() const;
    ControlGroup *group(const QString &name) const;
private slots:
    void controlAdded(Control *control);
    void controlRemoved(Control *control);

private:
    BackendManager();
    static BackendManager *s_instance;
    QList<Backend*> m_backends;
    QHash<Control::Category, ControlGroup*> m_groups;
};

#endif //BACKENDMANAGER_H
