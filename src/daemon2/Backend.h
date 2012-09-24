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
#ifndef BACKEND_H
#define BACKEND_H

#include <QtCore/QObject>
class Control;

class Backend : public QObject {
    Q_OBJECT
public:
    Backend(QObject *parent = 0);
    ~Backend();
    QList<Control*> controls() const;
    virtual bool open();
signals:
    void controlAdded(Control *control);
    void controlRemoved(Control *control);
protected:
    void registerControl(Control *control);
    void deregisterControl(Control *control);
private:
    QList<Control*> m_controls;
};

#endif //BACKEND_H
