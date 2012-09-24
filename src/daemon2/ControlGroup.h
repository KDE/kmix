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
#ifndef CONTROLGROUP_H
#define CONTROLGROUP_H

#include <QtCore/QObject>
#include <QtCore/QHash>

class Control;

class ControlGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName);
public:
    ControlGroup(const QString &displayName, QObject *parent = 0);
    ~ControlGroup();
    QString displayName() const;
    QStringList controls() const;
    Control *getControl(const QString &name) const;
public slots:
    void addControl(Control *control);
signals:
    void controlAdded(const QString &name) const;
private:
    QHash<QString, Control*> m_controls;
    QString m_displayName;
    static QAtomicInt s_id;
};

#endif //CONTROLGROUP_H
