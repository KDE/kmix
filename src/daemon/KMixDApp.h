//-*-C++-*-
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
#ifndef KMIXDAPP_H
#define KMIXDAPP_H

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>

class Control;

class KMixDApp : public QCoreApplication
{
    Q_OBJECT
    Q_PROPERTY(QStringList mixerGroups READ mixerGroups);
    Q_PROPERTY(QString masterControl READ masterControl);
    Q_PROPERTY(int masterVolume READ masterVolume WRITE setMasterVolume);
public:
    KMixDApp(int &argc, char **argv);
    ~KMixDApp();
    int start();
    void setMaster(int masterID);
    QStringList mixerGroups() const;
    QString masterControl() const;

    int masterVolume() const;
    void setMasterVolume(int v);
signals:
    void groupAdded(const QString &name);
    void groupRemoved(const QString &name);
    void masterChanged(const QString &path);
    void masterVolumeChanged();
private slots:
    void controlAdded(Control *);
    void controlRemoved(Control *);
private:
    Control *m_master;
};

#endif // KMIXDAPP_H
