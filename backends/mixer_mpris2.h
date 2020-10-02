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
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QMap>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>

#include "mixer_backend.h"

class MPrisControl : public QObject
{
  Q_OBJECT 

public:
  MPrisControl(QString id, QString busDestination);
  ~MPrisControl();

	enum DATA_ELEM
	{
		NONE = 0, NAME = 1, VOLUME = 2, ALL = 3
	};

  QDBusInterface* propertyIfc;
  QDBusInterface* playerIfc;

  
private:
	QString id;
	QString busDestination;
	QString name;
	int volume;

	int retrievedElems;

public:
	const QString& getId() const
	{
		return id;
	}

	const QString& getBusDestination() const
	{
		return busDestination;
	}

	const QString& getName() const
	{
		return name;
	}

	void setName(const QString& name)
	{
		retrievedElems |= MPrisControl::NAME;
		this->name = name;
	}

	int getVolume() const
	{
		return volume;
	}

	void setVolume(int volume)
	{
		retrievedElems |= MPrisControl::VOLUME;
		this->volume = volume;
	}

	bool isComplete()
	{
		return retrievedElems == MPrisControl::ALL;
	}



public slots:
  void trackChangedIncoming(QVariantMap msg);
  void onPropertyChange(QString,QVariantMap,QStringList);

signals:
  void volumeChanged(MPrisControl* mad, double);
  void playbackStateChanged(MPrisControl* mad, MediaController::PlayState);
};

class Mixer_MPRIS2 : public Mixer_Backend
{
  Q_OBJECT

//  friend class Mixer_MPRIS2_Thread;

public:
   Mixer_MPRIS2(Mixer *mixer, int device);
    virtual ~Mixer_MPRIS2();
    QString getDriverName() override;
    QString getId() const override { return _id; }

  int open() override;
  int close() override;
  int readVolumeFromHW( const QString& id, shared_ptr<MixDevice> ) override;
  int writeVolumeToHW( const QString& id, shared_ptr<MixDevice> ) override;
  void setEnumIdHW(const QString& id, unsigned int) override;
  unsigned int enumIdHW(const QString& id) override;
  bool needsPolling() override { return false; }

  int mediaPlay(QString id) override;
  int mediaPrev(QString id) override;
  int mediaNext(QString id) override;
  virtual int mediaControl(QString id, QString command);

  static MediaController::PlayState mprisPlayStateString2PlayState(const QString& playbackStatus);

public slots:
    void volumeChanged(MPrisControl *mad, double);
    void playbackStateChanged(MPrisControl* mad, MediaController::PlayState);

    void newMediaPlayer(QString name, QString oldOwner, QString newOwner);
    void addMprisControlAsync(QString arg1);
    void announceControlListAsync(QString streamId);

private slots:
    // asynchronous announce call slots
    void announceControlList();
    void announceGUI();
    void announceVolume();

    // Async QDBusPendingCallWatcher's
    void watcherMediaControl(QDBusPendingCallWatcher* watcher);
    void watcherPlugControlId(QDBusPendingCallWatcher* watcher);
    void watcherInitialVolume(QDBusPendingCallWatcher* watcher);
    void watcherInitialPlayState(QDBusPendingCallWatcher* watcher);
    void watcherDesktopFile(QDBusPendingCallWatcher* watcher);

private:
    // Helpers for the watchers
    MPrisControl* watcherHelperGetMPrisControl(QDBusPendingCallWatcher* watcher);


private:
//    void asyncAddMprisControl(QString busDestination);
//    void messageQueueThreadLoop();
    int addAllRunningPlayersAndInitHotplug();
    void volumeChangedInternal(shared_ptr<MixDevice> md, int volumePercentage);
	QString busDestinationToControlId(const QString& busDestination);

  QMap<QString,MPrisControl*> controls;
  QString _id;
};

#endif

