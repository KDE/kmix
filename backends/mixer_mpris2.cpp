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

#include <QDebug>
#include <QStringList>
#include <QDBusReply>
#include <QString>
#include <qvariant.h>

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

	m_mixerName = i18n("Playback Streams");
	_id = "Playback Streams";
	_mixer->setDynamic();
	addAllRunningPlayersAndInitHotplug();
	return 0;
}

int Mixer_MPRIS2::close()
{
	  m_isOpen = false;
	  closeCommon();
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

	kDebug() << "Send " << commandName << " to applicationId=" << applicationId;
	QDBusPendingReply<> repl2 =
		mad->playerIfc->asyncCall(commandName);


	QDBusPendingCallWatcher* watchMediaControlReply = new QDBusPendingCallWatcher(repl2, mad);
	connect(watchMediaControlReply, SIGNAL(finished(QDBusPendingCallWatcher *)), this, SLOT(mediaContolReplyIncoming(QDBusPendingCallWatcher *)));

	return 0; // Presume everything went well. Can't do more for ASYNC calls
}

void Mixer_MPRIS2::mediaContolReplyIncoming(QDBusPendingCallWatcher* watcher)
{
	QObject *obj = watcher->parent();
	MPrisControl* mad = qobject_cast<MPrisControl*>(obj);
	if (mad == 0)
	{
		kWarning() << "Ignoring unexpected Control Id";
		return;
	}
	QString id = mad->getId();
	QString busDestination = mad->getBusDestination();
	QString readableName = id; // Start with ID, but replace with reply (if exists)

	kDebug() << "Media control for id=" << id << ", busDestination" << busDestination << ", name= " << readableName;

	const QDBusMessage& msg = watcher->reply();
	if ( msg.type() == QDBusMessage::ErrorMessage )
	{
		kError(67100) << "ERROR in Media control operation, id=" << id << ": " << msg;
	}
}

/**
 * readVolumeFromHW() should be used only for hotplug (and even that should go away). Everything should operate via
 * the slot volumeChanged in the future.
 */
int Mixer_MPRIS2::readVolumeFromHW( const QString& id, shared_ptr<MixDevice> md)
{
	int volInt = 0;

	QList<QVariant> arg;
	arg.append(QString("org.mpris.MediaPlayer2.Player"));
	arg.append(QString("Volume"));
	MPrisControl* mad = controls.value(id);
//	QDBusMessage msg = mad->propertyIfc->callWithArgumentList(QDBus::Block, "Get", arg);

	QVariant v1 = QVariant(QString("org.mpris.MediaPlayer2.Player"));
	QVariant v2 = QVariant(QString("Volume"));

	QDBusPendingReply<QVariant > repl2 = mad->propertyIfc->asyncCall("Get", v1, v2);
	repl2.waitForFinished();
	QDBusMessage msg = repl2.reply();


	if ( msg.type() == QDBusMessage::ReplyMessage )
	{
		QList<QVariant> repl = msg.arguments();
		if ( ! repl.isEmpty() )
		{
			QVariant qv = repl.at(0);
			// We have to do some very ugly casting from QVariant to QDBusVariant to QVariant. This API totally sucks.
			QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(qv);
			QVariant result2 = dbusVariant.variant();
			volInt = result2.toFloat() *100;

			volumeChangedInternal(md, volInt);
			kDebug() << "changed vol" << volInt;
//			kDebug(67100) << "REPLY " << qv.type() << ": " << volInt;
		}
		else
		{
			kError(67100) << "ERROR GET " << id;
			return Mixer::ERR_READ;
		}

	}
	return 0;
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
	ControlManager::instance().announce(_mixer->id(), ControlChangeType::Volume, QString("MixerMPRIS2.volumeChanged"));
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
//	QDBusMessage msg = mad->propertyIfc->callWithArgumentList(QDBus::NoBlock, "Set", arg);

	QVariant v1 = QVariant(QString("org.mpris.MediaPlayer2.Player"));
	QVariant v2 = QVariant(QString("Volume"));
	QVariant v3 = QVariant::fromValue(QDBusVariant(volFloat));
//	QVariant v3 = QVariant(volFloat);

	//QDBusPendingReply<QVariant > repl2 =
	mad->propertyIfc->asyncCall("Set", v1, v2, v3);
	/*
	repl2.waitForFinished();
	QDBusMessage msg = repl2.reply();



	if ( msg.type() == QDBusMessage::ErrorMessage )
	{
		kError(67100) << "ERROR SET " << id << ": " << msg;
		return Mixer::ERR_WRITE;
	}
	*/
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
	bool ret = dbusConn.connect("", QString("/org/freedesktop/DBus"), "org.freedesktop.DBus", "NameOwnerChanged", this, SLOT(newMediaPlayer(QString,QString,QString)) );
	kDebug() << "Start listening for new Mediaplayers: "  << ret;

	/* Here is a small concurrency issue.
	 * If new players appear between registeredServiceNames() below and the connect() above these players *might* show up doubled in KMix.
	 * There is no simple solution (reversing could have the problem of not-adding), so we live for now with it.
	 */
	 
	/*
	 * Bug 311189: Introspecting via "dbusConn.interface()->registeredServiceNames()" does not work too well in
	 *   specific scenarios. Thus I now do a hand crafted 3-line asynchronous version of registeredServiceNames().
	 */
	QDBusInterface dbusIfc("org.freedesktop.DBus", "/org/freedesktop/DBus",
	                          "org.freedesktop.DBus", dbusConn);
	QDBusPendingReply<QStringList> repl = dbusIfc.asyncCall("ListNames");
	repl.waitForFinished(); // TODO Actually waitForFinished() is not "asynchronous enough"


	if ( repl.isValid() )
	{
		qDebug() << "Attaching Media Players";
		QString busDestination;
		foreach ( busDestination , repl.value() )
		{
			if ( busDestination.startsWith("org.mpris.MediaPlayer2") )
			{
				addMprisControl(busDestination);
				kDebug() << "Attached " << busDestination;
			}
		}
	}
	else
	{
		kError() << "Invalid reply while listing Media Players" << repl.error();
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
 * Add the MPRIS control designated by the DBUS busDestination
 * to the internal apps list.
 *
 * @param conn An open connection to the DBUS Session Bus
 * @param busDestination The DBUS busDestination, e.g. "org.mpris.MediaPlayer2.amarok"
 */
void Mixer_MPRIS2::addMprisControl(QString busDestination)
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

	// Create ASYNC DBUS queries for the new control
	QVariant v1 = QVariant(QString("org.mpris.MediaPlayer2"));
	QVariant v2 = QVariant(QString("Identity"));

	QDBusPendingReply<QVariant > repl2 = mad->propertyIfc->asyncCall("Get", v1, v2);
	QDBusPendingCallWatcher* watchIdentity = new QDBusPendingCallWatcher(repl2, mad);
	connect(watchIdentity, SIGNAL(finished(QDBusPendingCallWatcher *)), this, SLOT(plugControlIdIncoming(QDBusPendingCallWatcher *)));
}

void Mixer_MPRIS2::plugControlIdIncoming(QDBusPendingCallWatcher* watcher)
{
	QObject *obj = watcher->parent();
	MPrisControl* mad = qobject_cast<MPrisControl*>(obj);
	if (mad == 0)
	{
		kWarning() << "Ignoring unexpected Control Id";
		return;
	}
	QString id = mad->getId();
	QString busDestination = mad->getBusDestination();
	QString readableName = id; // Start with ID, but replace with reply (if exists)

	kDebug() << "Plugging id=" << id << ", busDestination" << busDestination << ", name= " << readableName;

	const QDBusMessage& msg = watcher->reply();
		if ( msg.type() == QDBusMessage::ReplyMessage )
		{
			QList<QVariant> repl = msg.arguments();
			if ( ! repl.isEmpty() )
			{
				QVariant qv = repl.at(0);
				// We have to do some very ugly casting from QVariant to QDBusVariant to QVariant. This API totally sucks.
				QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(qv);
				QVariant result2 = dbusVariant.variant();
				readableName = result2.toString();

				qDebug() << "REPLY " << result2.type() << ": " << readableName;
			}
		}
		else
		{
			qWarning() << "Error (" << msg.type() << "): " << msg.errorName() << " " << msg.errorMessage();
		}

			// TODO This hardcoded application list is a quick hack. It should be generalized.
			MixDevice::ChannelType ct = MixDevice::APPLICATION_STREAM;
			if (id.startsWith("amarok")) {
				ct = MixDevice::APPLICATION_AMAROK;
			}
			else if (id.startsWith("banshee")) {
				ct = MixDevice::APPLICATION_BANSHEE;
			}
			else if (id.startsWith("xmms")) {
				ct = MixDevice::APPLICATION_XMM2;
			}
			else if (id.startsWith("tomahawk")) {
				ct = MixDevice::APPLICATION_TOMAHAWK;
			}

			MixDevice* mdNew = new MixDevice(_mixer, id, readableName, ct);
			// MPRIS2 doesn't support an actual mute switch. Mute is defined as volume = 0.0
			// Thus we won't add the playback switch
			Volume* vol = new Volume( 100, 0, false, false);
			vol->addVolumeChannel(VolumeChannel(Volume::LEFT)); // MPRIS is only one control ("Mono")
			mdNew->addMediaPlayControl();
			mdNew->addMediaNextControl();
			mdNew->addMediaPrevControl();
			mdNew->setApplicationStream(true);
			mdNew->addPlaybackVolume(*vol);

			m_mixDevices.append( mdNew->addToPool() );

			QDBusConnection sessionBus = QDBusConnection::sessionBus();
			sessionBus.connect(busDestination, QString("/org/mpris/MediaPlayer2"), "org.freedesktop.DBus.Properties", "PropertiesChanged", mad, SLOT(volumeChangedIncoming(QString,QVariantMap,QStringList)) );
			connect(mad, SIGNAL(volumeChanged(MPrisControl*,double)), this, SLOT(volumeChanged(MPrisControl*,double)) );

			sessionBus.connect(busDestination, QString("/Player"), "org.freedesktop.MediaPlayer", "TrackChange", mad, SLOT(trackChangedIncoming(QVariantMap)) );
			volumeChanged(mad, mad->playerIfc->property("Volume").toDouble());
			// Push notifyToReconfigureControls to stack, so it will not be executed synchronously
			notifyToReconfigureControlsAsync(id);
}


void Mixer_MPRIS2::notifyToReconfigureControlsAsync(QString /*streamId*/)
{
	// currently we do not use the streamId
	QMetaObject::invokeMethod(this,
                              "notifyToReconfigureControls",
                              Qt::QueuedConnection);
}

void Mixer_MPRIS2::notifyToReconfigureControls()
{
    ControlManager::instance().announce(_mixer->id(), ControlChangeType::ControlList, getDriverName());
}

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
			addMprisControl(name);
		}
		else if ( !oldOwner.isEmpty() && newOwner.isEmpty())
		{
			kDebug() << "Mediaplayer unregisters: " << name;
			QString id = busDestinationToControlId(name);
			controls.remove(id);
			shared_ptr<MixDevice> md = m_mixDevices.get(id);
			if (md != 0)
			{
				// We know about the player that is unregistering => remove internally
				md->close();
				m_mixDevices.removeById(id);
				notifyToReconfigureControlsAsync(id);
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

/**
 * This slot is a simple proxy that enriches the DBUS signal with our data, which especially contains the id of the MixDevice.
 */
void MPrisControl::volumeChangedIncoming(QString /*ifc*/,QVariantMap msg ,QStringList /*sl*/)
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
		// "Stopped", "Playing", "Paused"
		kDebug() << "PlaybackStatus is now " << playbackStatus;
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
{}

QString Mixer_MPRIS2::getDriverName()
{
	return "MPRIS2";
}

QString MPRIS2_getDriverName()
{
	return "MPRIS2";
}

#include "mixer_mpris2.moc"
