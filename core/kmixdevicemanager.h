/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2006-2007 Christian Esken <esken@kde.org>
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

#ifndef KMIXDEVICEMANAGER_H
#define KMIXDEVICEMANAGER_H

#include <QObject>
#include <QMap>

#include "kmixcore_export.h"

class QTimer;


class KMIXCORE_EXPORT KMixDeviceManager : public QObject
{
  Q_OBJECT

    public:
        static KMixDeviceManager* instance();

        void initHotplug();
        void setHotpluggingBackends(const QString &backendName);
        QString getUDI(int devnum) const;

    Q_SIGNALS:
        void plugged(const char *driverName, const QString &udi, int devnum);
        void unplugged(const QString &udi);

    private:
        // Used only in this class, but must be available here
        // so that 'mPendingMap' can be defined below.
        enum PendingFlag
        {
                PluggedALSA = 0x01,
                PluggedOSS = 0x02,
                Unplugged = 0x04,
                UnplugOverride = 0x08
        };
        Q_DECLARE_FLAGS(PendingFlags, PendingFlag)

    private:
        KMixDeviceManager() = default;
        virtual ~KMixDeviceManager() = default;
        QString _hotpluggingBackend;
        QTimer *mHotplugTimer;

        QMap<int, PendingFlags> mPendingMap;

    private Q_SLOTS:
        void pluggedSlot(const QString &udi);
        void unpluggedSlot(const QString &udi);
        void slotTimer();
};

#endif
