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

#include "mixer_mpris2.h"
#include "core/mixer.h"
#include "core/ControlManager.h"
#include "core/GlobalConfig.h"

#include <QStringList>
#include <QDBusReply>
#include <QString>
#include <qvariant.h>

#include <KDebug>
#include <KLocale>

// Set the QDBUS_DEBUG env variable for debugging Qt DBUS calls.

Mixer_Backend* MPRIS2_getMixer(Mixer *mixer, int device )
{
	return new Mixer_MPRIS2(mixer, device );
}

Mixer_MPRIS2::Mixer_MPRIS2(Mixer *mixer, int device) : Mixer_Backend(mixer, device )
{
}


int Mixer_MPRIS2::open()
{
	if ( m_devnum !=  0 )
		return Mixer::ERR_OPEN;

	registerCard(i18n("Playback Streams"));
	_id = "Playback Streams";
	_mixer->setDynamic();
	return addAllRunningPlayersAndInitHotplug();
}

int Mixer_MPRIS2::close()
{
	  m_isOpen = false;
	  closeCommon();
	  qDeleteAll(controls);
	  controls.clear();
	 return 0;
}

int Mixer_MPRIS2::mediaPlay(QString id)
{
	return mediaControl(id, "PlayPause");
}

int Mixer_MPRIS2::mediaPrev(QString id)
{
	return mediaControl(id, "Previous");
}

int Mixer_MPRIS2::mediaNext(QString id)
{
	return mediaControl(id, "Next");
}

/**
 * Sends a media control command to the given application.
 * @param applicationId The MPRIS applicationId
 * @returns Always 0. Hint: Currently nobody uses the return code
 */
int Mixer_MPRIS2::mediaControl(QString applicationId, QString commandName)
{
	MPrisControl* mad = controls.value(applicationId);
	if ( mad == 0 )
	  return 0; // Might have disconnected recently => simply ignore command

	kDebug() << "Send " << commandName << " to id=" << applicationId;
	QDBusPendingReply<> repl2 =
		mad->playerIfc->asyncCall(commandName);


	QDBusPendingCallWatcher* watchMediaControlReply = new QDBusPendingCallWatcher(repl2, mad);
	connect(watchMediaControlReply, SIGNAL(finished(QDBusPendingCallWatcher *)), this, SLOT(watcherMediaControl(QDBusPendingCallWatcher *)));

	return 0; // Presume everything went well. Can't do more for ASYNC calls
}

void Mixer_MPRIS2::watcherMediaControl(QDBusPendingCallWatcher* watcher)
{
	MPrisControl* mprisCtl = watcherHelperGetMPrisControl(watcher);
	if (mprisCtl == 0)
	{
		return; // Reply for unknown media player. Probably "unplugged" (or not yet plugged)
	}

	// Actually the code below in this method is more or less just debugging
	const QDBusMessage& msg = watcher->reply();
	QString id = mprisCtl->getId();
	QString busDestination = mprisCtl->getBusDestination();
	kDebug() << "Media control for id=" << id << ", path=" << msg.path() << ", interface=" << msg.interface() << ", busDestination" << busDestination;
}

/**
 * readVolumeFromHW() should be used only for hotplug (and even that should go away). Everything should operate via
 * the slot volumeChanged in the future.
 */
int Mixer_MPRIS2::readVolumeFromHW( const QString& /*id*/, shared_ptr<MixDevice> /*md*/)
{
	// Everything is done by notifications => no code neccessary
	return Mixer::OK_UNCHANGED;
}


/**
 * A slot that processes data from the MPrisControl that emit the signal.
 *
 * @param The  emitting MPrisControl
 * @param newVolume The new volume
 */
void Mixer_MPRIS2::playbackStateChanged(MPrisControl* mad, MediaController::PlayState playState)
{
	shared_ptr<MixDevice> md = m_mixDevices.get(mad->getId());
	md->getMediaController()->setPlayState(playState);
	QMetaObject::invokeMethod(this, "announceGUI", Qt::QueuedConnection);
//	ControlManager::instance().announce(_mixer->id(), ControlChangeType::GUI, QString("MixerMPRIS2.playbackStateChanged"));
}


/**
 * A slot that processes data from the MPrisControl that emit the signal.
 *
 * @param The  emitting MPrisControl
 * @param newVolume The new volume
 */
void Mixer_MPRIS2::volumeChanged(MPrisControl* mad, double newVolume)
{
	shared_ptr<MixDevice> md = m_mixDevices.get(mad->getId());
	int volInt = newVolume *100;
	if (GlobalConfig::instance().data.debugVolume)
		kDebug() << "changed" << volInt;
	volumeChangedInternal(md, volInt);
}

void Mixer_MPRIS2::volumeChangedInternal(shared_ptr<MixDevice> md, int volumePercentage)
{
	if ( md->isVirtuallyMuted() && volumePercentage == 0)
	{
		// Special code path for virtual mute switches. Don't write back the volume if it is muted in the KMix GUI
		return;
	}

	Volume& vol = md->playbackVolume();
	vol.setVolume( Volume::LEFT, volumePercentage);
	md->setMuted(volumePercentage == 0);
	QMetaObject::invokeMethod(this, "announceVolume", Qt::QueuedConnection);
//	ControlManager::instance().announce(_mixer->id(), ControlChangeType::Volume, QString("MixerMPRIS2.volumeChanged"));
}

// The following is an example message for an incoming volume change:
/*
signal sender=:1.125 -> dest=(null destination) serial=503 path=/org/mpris/MediaPlayer2; interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
   string "org.mpris.MediaPlayer2.Player"
   array [
      dict entry(
         string "Volume"
         variant             double 0.81
      )
   ]
   array [
   ]
 */

/**
 * @overload
 *
 * @param id
 * @param md
 * @return
 */
int Mixer_MPRIS2::writeVolumeToHW( const QString& id, shared_ptr<MixDevice> md )
{
	Volume& vol = md->playbackVolume();
	double volFloat = 0;
	if ( ! md->isMuted() )
	{
		int volInt = vol.getVolume(Volume::LEFT);
		volFloat = volInt/100.0;
	}

	QList<QVariant> arg;
	arg.append(QString("org.mpris.MediaPlayer2.Player"));
	arg.append(QString("Volume"));
	arg << QVariant::fromValue(QDBusVariant(volFloat));

	MPrisControl* mad = controls.value(id);

	QVariant v1 = QVariant(QString("org.mpris.MediaPlayer2.Player"));
	QVariant v2 = QVariant(QString("Volume"));
	QVariant v3 = QVariant::fromValue(QDBusVariant(volFloat));
//	QVariant v3 = QVariant(volFloat);

	// I don't care too much for the reply, as I won't receive a result. Thus fire-and-forget here.
	mad->propertyIfc->asyncCall("Set", v1, v2, v3);
	return 0;
}

void Mixer_MPRIS2::setEnumIdHW(const QString&, unsigned int)
{
	// no enums in MPRIS
}

unsigned int Mixer_MPRIS2::enumIdHW(const QString&)
{
	// no enums in MPRIS
	return 0;
}

bool Mixer_MPRIS2::moveStream( const QString&, const QString&  )
{
	// not supported in MPRIS
	return false;
}


/**
 * Adds all currently running players and then starts listening
 * for changes (new players, and disappearing players).<br>
 *
 * @return int
 **/
int Mixer_MPRIS2::addAllRunningPlayersAndInitHotplug()
{
	QDBusConnection dbusConn = QDBusConnection::sessionBus();
	if (! dbusConn.isConnected() )
	{
		kError(67100) <<  "Cannot connect to the D-Bus session bus.\n"
				<< "To start it, run:\n"
				<<"\teval `dbus-launch --auto-syntax`\n";
		return Mixer::ERR_OPEN;
	}

	// Start listening for new Mediaplayers
	bool connected = dbusConn.connect("", QString("/org/freedesktop/DBus"), "org.freedesktop.DBus", "NameOwnerChanged", this, SLOT(newMediaPlayer(QString,QString,QString)) );
	if (!connected)
	{
		kWarning() << "MPRIS2 hotplug init failure. New Media Players will not be detected.";
	}

	/* Here is a small concurrency issue.
	 * If new players appear between registeredServiceNames() below and the connect() above these players *might* show up doubled in KMix.
	 * There is no simple solution (reversing could have the problem of not-adding), so we live for now with it.
	 */
	 
	/*
	 * Bug 311189: Introspecting via "dbusConn.interface()->registeredServiceNames()" does not work too well.
	 * Comment: I am not so sure that registeredServiceNames() is really an issue. It is more likely
	 *  in a later step, when talking to the probed apps. Still, I now do a hand crafted 3-line version of
	 *  registeredServiceNames() via "ListNames", so I can later more easily change to async.
	 */
	QDBusInterface dbusIfc("org.freedesktop.DBus", "/org/freedesktop/DBus",
	                          "org.freedesktop.DBus", dbusConn);
	QDBusPendingReply<QStringList> repl = dbusIfc.asyncCall("ListNames");
	repl.waitForFinished();

	if (! repl.isValid() )
	{
		kError() << "Invalid reply while listing Media Players. MPRIS2 players will not be available." << repl.error();
		return 1;
	}

	QString busDestination;
	foreach ( busDestination , repl.value() )
	{
		if ( busDestination.startsWith("org.mpris.MediaPlayer2") )
		{
			addMprisControlAsync(busDestination);
			kDebug() << "MPRIS2: Attached media player on busDestination=" << busDestination;
		}
	}

	return 0;
}

QString Mixer_MPRIS2::busDestinationToControlId(const QString& busDestination)
{
	const QString prefix = "org.mpris.MediaPlayer2.";
	if (! busDestination.startsWith(prefix))
	{
		kWarning() << "Ignoring unsupported control, busDestination=" << busDestination;
		return QString();
	}

	return busDestination.mid(prefix.length());
}

/**
 * Asynchronously add the MPRIS control designated by the DBUS busDestination.
 * to the internal apps list.
 *
 * @param conn An open connection to the DBUS Session Bus
 * @param busDestination The DBUS busDestination, e.g. "org.mpris.MediaPlayer2.amarok"
 */
void Mixer_MPRIS2::addMprisControlAsync(QString busDestination)
{
	// -1- Create a MPrisControl. Its fields will be filled partially here, partially via ASYNC DUBUS replies
	QString id = busDestinationToControlId(busDestination);
	kDebug() << "Get control of busDestination=" << busDestination << "id=" << id;

	QDBusConnection conn = QDBusConnection::sessionBus();
	QDBusInterface *qdbiProps  = new QDBusInterface(QString(busDestination), QString("/org/mpris/MediaPlayer2"), "org.freedesktop.DBus.Properties", conn, this);
	QDBusInterface *qdbiPlayer = new QDBusInterface(QString(busDestination), QString("/org/mpris/MediaPlayer2"), "org.mpris.MediaPlayer2.Player", conn, this);

	// -2- Add the control to our official control list
	MPrisControl* mad = new MPrisControl(id, busDestination);
	mad->propertyIfc = qdbiProps;
	mad->playerIfc = qdbiPlayer;
	controls.insert(id, mad);


	/*
	 * WTF: - asyncCall("Get", arg)                          : returns an error message (see below)
	 *      - asyncCallWithArgumentList("Get", arg)          : returns an error message (see below)
	 *      - callWithArgumentList(QDBus::Block, "Get", arg) : works
	 *      - syncCall("Get", v1, v2)                        : works
	 *
	 * kmix(13543) Mixer_MPRIS2::addMPrisControl: (marok), msg2= QDBusMessage(type=Error, service=":1.44", error name="org.freedesktop.DBus.Error.UnknownMethod", error message="No such method 'Get' in interface 'org.freedesktop.DBus.Properties' at object path '/org/mpris/MediaPlayer2' (signature 'av')", signature="s", contents=("No such method 'Get' in interface 'org.freedesktop.DBus.Properties' at object path '/org/mpris/MediaPlayer2' (signature 'av')") ) , isValid= false , isFinished= true , isError= true
	 *
	 * This behavior is total counter-intuitive :-(((
	 */

	// Create ASYNC DBUS queries for the new control. This effectively starts a chain of async DBUS commands.
	QVariant v1 = QVariant(QString("org.mpris.MediaPlayer2"));
	QVariant v2 = QVariant(QString("Identity"));
	QDBusPendingReply<QVariant > repl2 = mad->propertyIfc->asyncCall("Get", v1, v2);
	QDBusPendingCallWatcher* watchIdentity = new QDBusPendingCallWatcher(repl2, mad);
	connect(watchIdentity, SIGNAL(finished(QDBusPendingCallWatcher *)), this, SLOT(watcherPlugControlId(QDBusPendingCallWatcher *)));
}

MixDevice::ChannelType Mixer_MPRIS2::getChannelTypeFromPlayerId(const QString& id)
{
	// TODO This hardcoded application list is a quick hack. It should be generalized.
	MixDevice::ChannelType ct = MixDevice::APPLICATION_STREAM;
	if (id.startsWith("amarok"))
	{
		ct = MixDevice::APPLICATION_AMAROK;
	}
	else if (id.startsWith("banshee"))
	{
		ct = MixDevice::APPLICATION_BANSHEE;
	}
	else if (id.startsWith("vlc"))
	{
		ct = MixDevice::APPLICATION_VLC;
	}
	else if (id.startsWith("xmms"))
	{
		ct = MixDevice::APPLICATION_XMM2;
	}
	else if (id.startsWith("tomahawk"))
	{
		ct = MixDevice::APPLICATION_TOMAHAWK;
	}
	else if (id.startsWith("clementine"))
	{
		ct = MixDevice::APPLICATION_CLEMENTINE;
	}

	return ct;
}

void Mixer_MPRIS2::watcherInitialVolume(QDBusPendingCallWatcher* watcher)
{
	MPrisControl* mprisCtl = watcherHelperGetMPrisControl(watcher);
	if (mprisCtl == 0)
		return; // Reply for unknown media player. Probably "unplugged" (or not yet plugged)

	const QDBusMessage& msg = watcher->reply();
	QList<QVariant> repl = msg.arguments();
	if ( ! repl.isEmpty() )
	{
		QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(repl.at(0));
		QVariant result2 = dbusVariant.variant();
		double volume = result2.toDouble();
		volumeChanged(mprisCtl, volume);
	}

	watcher->deleteLater();
}

void Mixer_MPRIS2::watcherInitialPlayState(QDBusPendingCallWatcher* watcher)
{
	MPrisControl* mprisCtl = watcherHelperGetMPrisControl(watcher);
	if (mprisCtl == 0)
		return; // Reply for unknown media player. Probably "unplugged" (or not yet plugged)

	const QDBusMessage& msg = watcher->reply();
	QList<QVariant> repl = msg.arguments();
	if ( ! repl.isEmpty() )
	{
		QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(repl.at(0));
		QVariant result2 = dbusVariant.variant();
		QString playbackStateString = result2.toString();

		MediaController::PlayState playState = Mixer_MPRIS2::mprisPlayStateString2PlayState(playbackStateString);
		playbackStateChanged(mprisCtl, playState);
	}

	watcher->deleteLater();
}


/**
 * Convenience method for the watcher*() methods.
 * Returns the MPrisControl that is parent of the given watcher, if the reply is valid. In this case you can
 * use the result and call watcher->deleteLater() after processing the result.
 *
 * Otherwise 0 is returned, and watcher->deleteLater() is called. <b>Important</b> You must call watcher->deleteLater()
 * yourself for the other (normal/good) case.
 *
 * @param watcher
 * @return
 */
MPrisControl* Mixer_MPRIS2::watcherHelperGetMPrisControl(QDBusPendingCallWatcher* watcher)
{
	const QDBusMessage& msg = watcher->reply();
	if ( msg.type() == QDBusMessage::ReplyMessage )
	{
		QObject* obj = watcher->parent();
		MPrisControl* mad = qobject_cast<MPrisControl*>(obj);
		if (mad != 0)
		{
			return mad;
		}
		kWarning() << "Ignoring unexpected Control Id. object=" << obj;
	}

	else if ( msg.type() == QDBusMessage::ErrorMessage )
	{
		kError() << "ERROR in Media control operation, path=" << msg.path() << ", msg=" << msg;
	}


	watcher->deleteLater();
	return 0;
}

void Mixer_MPRIS2::watcherPlugControlId(QDBusPendingCallWatcher* watcher)
{
	MPrisControl* mprisCtl = watcherHelperGetMPrisControl(watcher);
	if (mprisCtl == 0)
	{
		return; // Reply for unknown media player. Probably "unplugged" (or not yet plugged)
	}

	const QDBusMessage& msg = watcher->reply();
	QString id = mprisCtl->getId();
	QString busDestination = mprisCtl->getBusDestination();
	QString readableName = id; // Start with ID, but replace with reply (if exists)

	kDebug() << "Plugging id=" << id << ", busDestination" << busDestination << ", name= " << readableName;

	QList<QVariant> repl = msg.arguments();
	if ( ! repl.isEmpty() )
	{
		// We have to do some very ugly casting from QVariant to QDBusVariant to QVariant. This API totally sucks.
		QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(repl.at(0));
		QVariant result2 = dbusVariant.variant();
		readableName = result2.toString();

//			kDebug() << "REPLY " << result2.type() << ": " << readableName;

		MixDevice::ChannelType ct = getChannelTypeFromPlayerId(id);
		MixDevice* mdNew = new MixDevice(_mixer, id, readableName, ct);
		// MPRIS2 doesn't support an actual mute switch. Mute is defined as volume = 0.0
		// Thus we won't add the playback switch
		Volume* vol = new Volume( 100, 0, false, false);
		vol->addVolumeChannel(VolumeChannel(Volume::LEFT)); // MPRIS is only one control ("Mono")
		MediaController* mediaContoller = mdNew->getMediaController();
		mediaContoller->addMediaPlayControl();
		mediaContoller->addMediaNextControl();
		mediaContoller->addMediaPrevControl();
		mdNew->setApplicationStream(true);
		mdNew->addPlaybackVolume(*vol);

		m_mixDevices.append( mdNew->addToPool() );

		delete vol; // vol is only temporary. mdNew has its own volume object. => delete

		QDBusConnection sessionBus = QDBusConnection::sessionBus();
		sessionBus.connect(busDestination, QString("/org/mpris/MediaPlayer2"), "org.freedesktop.DBus.Properties", "PropertiesChanged", mprisCtl, SLOT(onPropertyChange(QString,QVariantMap,QStringList)) );
		connect(mprisCtl, SIGNAL(volumeChanged(MPrisControl*,double)), this, SLOT(volumeChanged(MPrisControl*,double)) );
		connect(mprisCtl, SIGNAL(playbackStateChanged(MPrisControl*,MediaController::PlayState)), SLOT (playbackStateChanged(MPrisControl*,MediaController::PlayState)) );

		sessionBus.connect(busDestination, QString("/Player"), "org.freedesktop.MediaPlayer", "TrackChange", mprisCtl, SLOT(trackChangedIncoming(QVariantMap)) );

		// The following line is evil: mad->playerIfc->property("Volume") is in fact a synchronous call, and
		// sync calls are strictly forbidden, see bug 317926
		//volumeChanged(mad, mad->playerIfc->property("Volume").toDouble());


		// --- Query initial state --------------------------------------------------------------------------------
		QVariant v1 = QVariant(QString("org.mpris.MediaPlayer2.Player"));

		QVariant v2 = QVariant(QString("Volume"));
		QDBusPendingReply<QVariant > repl2 = mprisCtl->propertyIfc->asyncCall("Get", v1, v2);
		QDBusPendingCallWatcher* watcherOutgoing = new QDBusPendingCallWatcher(repl2, mprisCtl);
		connect(watcherOutgoing, SIGNAL(finished(QDBusPendingCallWatcher *)), this, SLOT(watcherInitialVolume(QDBusPendingCallWatcher *)));

		v2 = QVariant(QString("PlaybackStatus"));
		repl2 = mprisCtl->propertyIfc->asyncCall("Get", v1, v2);
		watcherOutgoing = new QDBusPendingCallWatcher(repl2, mprisCtl);
		connect(watcherOutgoing, SIGNAL(finished(QDBusPendingCallWatcher *)), this, SLOT(watcherInitialPlayState(QDBusPendingCallWatcher *)));

		// Push notifyToReconfigureControls to stack, so it will not be executed synchronously
		announceControlListAsync(id);
	}

	watcher->deleteLater();
}


// -----------------------------------------------------------------------------------------------------------
// ASYNC announce slots, including convenience wrappers
// -----------------------------------------------------------------------------------------------------------
/**
 * Convenience wrapper to do the ASYNC call to #announceControlList()
 * @param
 */
void Mixer_MPRIS2::announceControlListAsync(QString /*streamId*/)
{
	// currently we do not use the streamId
	QMetaObject::invokeMethod(this, "announceControlList", Qt::QueuedConnection);
}

void Mixer_MPRIS2::announceControlList()
{
    ControlManager::instance().announce(_mixer->id(), ControlChangeType::ControlList, getDriverName());
}

void Mixer_MPRIS2::announceGUI()
{
    ControlManager::instance().announce(_mixer->id(), ControlChangeType::GUI, getDriverName());
}

void Mixer_MPRIS2::announceVolume()
{
    ControlManager::instance().announce(_mixer->id(), ControlChangeType::Volume, getDriverName());
}

// -----------------------------------------------------------------------------------------------------------


/**
 * Handles the hotplug of new MPRIS2 enabled Media Players
 */
void Mixer_MPRIS2::newMediaPlayer(QString name, QString oldOwner, QString newOwner)
{
	if ( name.startsWith("org.mpris.MediaPlayer2") )
	{
		if ( oldOwner.isEmpty() && !newOwner.isEmpty())
		{
			kDebug() << "Mediaplayer registers: " << name;
			addMprisControlAsync(name);
		}
		else if ( !oldOwner.isEmpty() && newOwner.isEmpty())
		{
			QString id = busDestinationToControlId(name);
			kDebug() << "Mediaplayer unregisters: " << name << " , id=" << id;

			// -1- Remove Mediaplayer connection
			if (controls.contains(id))
			{
				const MPrisControl *control = controls.value(id);
				QObject::disconnect(control,0,0,0);
				controls.remove(id);
			}

			// -2- Remove MixDevice from internal list
			shared_ptr<MixDevice> md = m_mixDevices.get(id);
			if (md)
			{
				// We know about the player that is unregistering => remove internally
				md->close();
				m_mixDevices.removeById(id);
				announceControlListAsync(id);
				kDebug() << "MixDevice 4 useCount=" << md.use_count();
			}
		}
		else
		{
			kWarning() << "Mediaplayer has registered under a new name. This is currently not supported by KMix";
		}
	}

}

/**
 * This slot is a simple proxy that enriches the DBUS signal with our data, which especially contains the id of the MixDevice.
 */
void MPrisControl::trackChangedIncoming(QVariantMap /*msg*/)
{
	kDebug() << "Track changed";
}

MediaController::PlayState Mixer_MPRIS2::mprisPlayStateString2PlayState(const QString& playbackStatus)
{
	MediaController::PlayState playState;
	if (playbackStatus == "Playing")
	{
		playState = MediaController::PlayPlaying;
	}
	else if (playbackStatus == "Stopped")
	{
		playState = MediaController::PlayStopped;
	}
	else if (playbackStatus == "Paused")
	{
		playState = MediaController::PlayPaused;
	}

	return playState;
}

/**
 * This slot is a simple proxy that enriches the DBUS signal with our data, which especially contains the id of the MixDevice.
 */
void MPrisControl::onPropertyChange(QString /*ifc*/,QVariantMap msg ,QStringList /*sl*/)
{
	QMap<QString, QVariant>::iterator v = msg.find("Volume");
	if (v != msg.end() )
	{
		double volDouble = v.value().toDouble();
		kDebug(67100) << "volumeChanged incoming: vol=" << volDouble;
		emit volumeChanged( this, volDouble);
	}

	v = msg.find("PlaybackStatus");
	if (v != msg.end() )
	{
		QString playbackStatus = v.value().toString();
		MediaController::PlayState playState = Mixer_MPRIS2::mprisPlayStateString2PlayState(playbackStatus);
		kDebug() << "PlaybackStatus is now " << playbackStatus;

		emit playbackStateChanged(this, playState);
	}
}


Mixer_MPRIS2::~Mixer_MPRIS2()
{
	close();
}

MPrisControl::MPrisControl(QString id, QString busDestination)
 : propertyIfc(0)
 , playerIfc(0)

{
	volume = 0;
	this->id = id;
	this->busDestination = busDestination;
	retrievedElems = MPrisControl::NONE;
}

MPrisControl::~MPrisControl()
{
	delete propertyIfc;
	delete playerIfc;
}

QString Mixer_MPRIS2::getDriverName()
{
	return "MPRIS2";
}

QString MPRIS2_getDriverName()
{
	return "MPRIS2";
}

#include "mixer_mpris2.moc"
