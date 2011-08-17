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

//#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QDBusReply>


#include <QString>

// Set the QDBUS_DEBUG env variable for debugging Qt DBUS calls.

Mixer_Backend* MPRIS2_getMixer(Mixer *mixer, int device )
{

	Mixer_Backend *l_mixer;

	l_mixer = new Mixer_MPRIS2(mixer,  device );
	return l_mixer;
}

Mixer_MPRIS2::Mixer_MPRIS2(Mixer *mixer, int device) : Mixer_Backend(mixer,  device )
{
	//	run();
}


int Mixer_MPRIS2::open()
{
	if ( m_devnum !=  0 )
		return Mixer::ERR_OPEN;

	_mixer->setDynamic();
	run();
	return 0;
}

int Mixer_MPRIS2::close()
{
	return 0;
}

QString Mixer_MPRIS2::getBusDestination(const QString& id)
{
	return QString("org.mpris.MediaPlayer2.").append(id);
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

int Mixer_MPRIS2::mediaControl(QString id, QString commandName)
{
	kDebug() << commandName << " " << id;
	QList<QVariant> arg;
	//     arg.append(QString("org.mpris.MediaPlayer2.Player"));
	//     arg.append(QString("PlayPause"));

	MPrisAppdata* mad = apps.value(id);
	QDBusMessage msg = mad->playerIfc->callWithArgumentList(QDBus::NoBlock, commandName, arg);
	if ( msg.type() == QDBusMessage::ErrorMessage )
	{
		kError(67100) << "ERROR SET " << id << ": " << msg;
		return Mixer::ERR_WRITE;
	}
	return 0;
}


int Mixer_MPRIS2::readVolumeFromHW( const QString& id, MixDevice * md)
{

	int volInt = 0;

	QList<QVariant> arg;
	arg.append(QString("org.mpris.MediaPlayer2.Player"));
	arg.append(QString("Volume"));
	MPrisAppdata* mad = apps.value(id);
	QDBusMessage msg = mad->propertyIfc->callWithArgumentList(QDBus::Block, "Get", arg);
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

			Volume& vol = md->playbackVolume();
			md->setMuted(volInt == 0);
			vol.setVolume( Volume::LEFT, volInt);
			kDebug(67100) << "REPLY " << qv.type() << ": " << volInt << ": "<< vol;
		}
		else
		{
			kError(67100) << "ERROR GET " << id;
			return Mixer::ERR_READ;
		}

	}
	return 0;
}

int Mixer_MPRIS2::writeVolumeToHW( const QString& id, MixDevice *md )
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

	MPrisAppdata* mad = apps.value(id);
	QDBusMessage msg = mad->propertyIfc->callWithArgumentList(QDBus::NoBlock, "Set", arg);
	if ( msg.type() == QDBusMessage::ErrorMessage )
	{
		kError(67100) << "ERROR SET " << id << ": " << msg;
		return Mixer::ERR_WRITE;
	}
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
 * @brief Test method
 *
 * @return int
 **/
int Mixer_MPRIS2::run()
{
	QDBusConnection dbusConn = QDBusConnection::sessionBus();
	if (! dbusConn.isConnected() )
	{
		kError(67100) <<  "Cannot connect to the D-Bus session bus.\n"
				<< "To start it, run:\n"
				<<"\teval `dbus-launch --auto-syntax`\n";
		return Mixer::ERR_OPEN;
	}

	kDebug(67100) << "--- B ---------------------------------";
	QDBusReply<QStringList> repl = dbusConn.interface()->registeredServiceNames();

	kDebug(67100) << "--- C ---------------------------------";
	if ( repl.isValid() )
	{
		kDebug(67100) << "--- D ---------------------------------";

		QStringList result = repl.value();
		QString s;
		foreach (  s , result )
		{
			if ( s.startsWith("org.mpris.MediaPlayer2") )
			{
				getMprisControl(dbusConn, s);

			}
		}
	}


	return 0;
}



void Mixer_MPRIS2::getMprisControl(QDBusConnection& conn, QString busDestination)
{
	int lastDot = busDestination.lastIndexOf('.');
	QString id = ( lastDot == -1 ) ? busDestination : busDestination.mid(lastDot+1);
	kDebug(67100) << "Get control of " << busDestination << "id=" << id;
	QDBusInterface *qdbiProps  = new QDBusInterface(QString(busDestination), QString("/org/mpris/MediaPlayer2"), "org.freedesktop.DBus.Properties", conn, this);
	QDBusInterface *qdbiPlayer = new QDBusInterface(QString(busDestination), QString("/org/mpris/MediaPlayer2"), "org.mpris.MediaPlayer2.Player", conn, this);

	MPrisAppdata* mad = new MPrisAppdata();
	mad->id = id;
	mad->propertyIfc = qdbiProps;
	mad->playerIfc = qdbiPlayer;

	apps.insert(id, mad);

	QList<QVariant> arg;
	arg.append(QString("org.mpris.MediaPlayer2"));
	arg.append(QString("Identity"));
	QDBusMessage msg = mad->propertyIfc->callWithArgumentList(QDBus::Block, "Get", arg);

	//msg = conn.call(query, QDBus::Block, 5);
	if ( msg.type() == QDBusMessage::ReplyMessage )
	{
		QList<QVariant> repl = msg.arguments();
		if ( ! repl.isEmpty() )
		{
			QVariant qv = repl.at(0);
			// We have to do some very ugly casting from QVariant to QDBusVariant to QVariant. This API totally sucks.
			QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(qv);
			QVariant result2 = dbusVariant.variant();
			QString readableName = result2.toString();

			qDebug() << "REPLY " << result2.type() << ": " << readableName;

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

			MixDevice* md = new MixDevice(_mixer, id, readableName, ct);
			// MPRIS2 doesn't support an actual mute switch. Mute is defined as volume = 0.0
			// Thus we won't add the playback switch
			Volume* vol = new Volume( 100, 0, false, false);
			vol->addVolumeChannel(VolumeChannel(Volume::LEFT)); // MPRIS is only one control ("Mono")
			md->addMediaPlayControl();
			md->addMediaNextControl();
			md->addMediaPrevControl();
			md->setApplicationStream(true);
			md->addPlaybackVolume(*vol);
			m_mixDevices.append( md );

			//	conn.connect("", QString("/org/mpris/MediaPlayer2"), "org.freedesktop.DBus.Properties", "PropertiesChanged", mad, SLOT(volumeChangedIncoming(QString,QList<QVariant>)) );
			conn.connect("", QString("/org/mpris/MediaPlayer2"), "org.freedesktop.DBus.Properties", "PropertiesChanged", mad, SLOT(volumeChangedIncoming(QString,QVariantMap,QStringList)) );
			connect(mad, SIGNAL(volumeChanged(MPrisAppdata*,double)), this, SLOT(volumeChanged(MPrisAppdata*,double)) );

		}
	}
	else
	{
		qWarning() << "Error (" << msg.type() << "): " << msg.errorName() << " " << msg.errorMessage();
	}
}


/**
 * This slot is a simple proxy that enriches the DBUS signal with our data, which especially contains the id of the MixDevice.
 */
void MPrisAppdata::volumeChangedIncoming(QString ifc,QVariantMap msg ,QStringList sl)
{
	QMap<QString, QVariant>::iterator v = msg.find("Volume");
	if (v != msg.end() )
	{
		double volDouble = v.value().toDouble();
		emit volumeChanged( this, volDouble);
	}
}



void Mixer_MPRIS2::volumeChanged(MPrisAppdata* mad, double newVolume)
{
	int volInt = newVolume *100;
	kDebug(67100) << "volumeChanged: " << mad->id << ": " << volInt;
	MixDevice * md = m_mixDevices.get(mad->id);
	Volume& vol = md->playbackVolume();
	vol.setVolume( Volume::LEFT, volInt);
	md->setMuted(volInt == 0);
	emit controlChanged();
	//  md->playbackVolume().setVolume(vol);
}

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

Mixer_MPRIS2::~Mixer_MPRIS2()
{
}

MPrisAppdata::MPrisAppdata()
{}

MPrisAppdata::~MPrisAppdata()
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