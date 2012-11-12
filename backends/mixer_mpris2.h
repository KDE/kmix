/**
 * KMix -- MPRIS2 backend
 *
 * Copyright (C) 2011 Christian Esken <esken@kde.org>
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
 *
 */

#ifndef Mixer_MPRIS2_H
#define Mixer_MPRIS2_H

#include <QString>
#include <QMainWindow>
#include <QtDBus>
#include <QDBusInterface>
#include <QMap>

#include "mixer_backend.h"

class MPrisAppdata : public QObject
{
  Q_OBJECT 
public:
  MPrisAppdata();
  ~MPrisAppdata();
  QString id;
  QDBusInterface* propertyIfc;
  QDBusInterface* playerIfc;

public slots:
  void trackChangedIncoming(QVariantMap msg);
  void volumeChangedIncoming(QString,QVariantMap,QStringList);
  
signals:
  void volumeChanged(MPrisAppdata* mad, double);
};



class Mixer_MPRIS2 : public Mixer_Backend
{
  Q_OBJECT
public:
   Mixer_MPRIS2(Mixer *mixer, int device);
    virtual ~Mixer_MPRIS2();
    void addMprisControl(QDBusConnection& conn, QString arg1);
    QString getDriverName();
    virtual QString getId() const { return _id; };

  virtual int open();
  virtual int close();
  virtual int readVolumeFromHW( const QString& id, shared_ptr<MixDevice> );
  virtual int writeVolumeToHW( const QString& id, shared_ptr<MixDevice> );
  virtual void setEnumIdHW(const QString& id, unsigned int);
  virtual unsigned int enumIdHW(const QString& id);
  virtual bool moveStream( const QString& id, const QString& destId );
  virtual bool needsPolling() { return false; }

  virtual int mediaPlay(QString id);
  virtual int mediaPrev(QString id);
  virtual int mediaNext(QString id);
  virtual int mediaControl(QString id, QString command);

public slots:
    void volumeChanged(MPrisAppdata *mad, double);
    void newMediaPlayer(QString name, QString oldOwner, QString newOwner);
private:
    int addAllRunningPlayersAndInitHotplug();
    void notifyToReconfigureControls();
    void volumeChangedInternal(shared_ptr<MixDevice> md, int volumePercentage);
    
  QMap<QString,MPrisAppdata*> apps;
  QString _id;
};

#endif

