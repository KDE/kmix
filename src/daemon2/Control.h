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
#ifndef CONTROL_H
#define CONTROL_H

#include <QtCore/QObject>
#include <QtCore/QMap>

class Control : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName);
    Q_PROPERTY(QString iconName READ iconName);
    Q_PROPERTY(bool mute READ isMuted WRITE setMute NOTIFY muteChanged);
    Q_PROPERTY(bool canMute READ canMute);
    Q_PROPERTY(int channels READ channels);
    Q_PROPERTY(Category category READ category);
public:
    typedef enum {
        FrontLeft,
        FrontRight,
        Center,
        RearLeft,
        RearRight,
        Subwoofer
    } Channel;
    typedef enum {
        OutputStream,
        InputStream,
        HardwareInput,
        HardwareOutput
    } Category;
    Control(Category, QObject *parent = 0);
    ~Control();
    virtual QString displayName() const = 0;
    virtual QString iconName() const = 0;
    virtual int channels() const = 0;
    virtual int getVolume(Channel c) const = 0;
    virtual void setVolume(Channel c, int v) = 0;
    virtual bool isMuted() const = 0;
    virtual void setMute(bool yes) = 0;
    virtual bool canMute() const = 0;
    virtual Category category() const;
    int id() const;

    int getVolume(int i) const {return getVolume((Channel)i);}
    void setVolume(int c, int v) {setVolume((Channel)c, v);}
signals:
    void volumeChanged(int c);
    void muteChanged(bool muted);
    void removed();
private:
    Category m_category;
    int m_id;
    static QAtomicInt s_id;
};

#endif //CONTROL_H