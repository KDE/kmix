/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2011 Igor Poboiko <igor.poboiko@gmail.com>
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

#include "mixerengine.h"
#include "mixset_interface.h"
#include "mixer_interface.h"
#include "control_interface.h"
#include "mixerservice.h"

#include <QIcon>
#include <QTimer>


const QString MixerEngine::KMIX_DBUS_SERVICE = "org.kde.kmix";
const QString MixerEngine::KMIX_DBUS_PATH = "/Mixers";

MixerEngine::MixerEngine(QObject *parent, const QVariantList &args)
	: Plasma::DataEngine(parent, args)
	, m_kmix(0)
{
	Q_UNUSED(args)

	interface = QDBusConnection::sessionBus().interface();
	watcher = new QDBusServiceWatcher( this );
	watcher->addWatchedService( KMIX_DBUS_SERVICE );
	watcher->setConnection( QDBusConnection::sessionBus() );
	watcher->setWatchMode( QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration );
	connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &MixerEngine::slotServiceRegistered);
	connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &MixerEngine::slotServiceUnregistered);
	init();
}

MixerEngine::~MixerEngine()
{
	// Cleanup
	delete watcher;
	// it is bad idea to call removeSource() here
	clearInternalData(false);
	delete m_kmix;
}

QStringList MixerEngine::sources() const
{
	QStringList sources;
	sources << "Mixers";
	return sources;
}

void MixerEngine::init()
{
	getInternalData();
}

MixerInfo* MixerEngine::createMixerInfo( const QString& dbusPath )
{
	MixerInfo* curmi = new MixerInfo;
	curmi->iface = new OrgKdeKMixMixerInterface( KMIX_DBUS_SERVICE, dbusPath,
					QDBusConnection::sessionBus(), this );
	curmi->id = curmi->iface->id();
	curmi->dbusPath = dbusPath;
	curmi->updateRequired = false;
	curmi->unused = false;
	curmi->connected = false;
	QDBusConnection::sessionBus().connect( KMIX_DBUS_SERVICE, dbusPath,
			"org.kde.KMix.Mixer", "changed",
			this, SLOT(slotControlsReconfigured()) );
	m_mixers.insert( dbusPath, curmi );
	return curmi;
}

ControlInfo* MixerEngine::createControlInfo( const QString& mixerId, const QString& dbusPath )
{
	ControlInfo* curci = new ControlInfo;
	curci->iface = new OrgKdeKMixControlInterface( KMIX_DBUS_SERVICE, dbusPath,
						QDBusConnection::sessionBus(), this );
	curci->mixerId = mixerId;
	curci->id = curci->iface->id();
	curci->dbusPath = dbusPath;
	curci->updateRequired = false;
	curci->unused = false;
	m_controls.insert(mixerId, curci);
	return curci;
}

void MixerEngine::getInternalData()
{
	clearInternalData(true);
	if ( !interface->isServiceRegistered( KMIX_DBUS_SERVICE ) )
		return;
	if ( !m_kmix )
	{
		m_kmix = new OrgKdeKMixMixSetInterface( KMIX_DBUS_SERVICE, KMIX_DBUS_PATH,
				QDBusConnection::sessionBus(), this );
		QDBusConnection::sessionBus().connect( KMIX_DBUS_SERVICE, KMIX_DBUS_PATH,
				"org.kde.KMix.MixSet", "mixersChanged",
				this, SLOT(slotMixersChanged()) );
		QDBusConnection::sessionBus().connect( KMIX_DBUS_SERVICE, KMIX_DBUS_PATH,
				"org.kde.KMix.MixSet", "masterChanged",
				this, SLOT(slotMasterChanged()) );
	}

	for (const QString &path : m_kmix->mixers())
	{
		const MixerInfo *curmi = createMixerInfo( path );
		for (const QString &controlPath : curmi->iface->controls())
		{
			createControlInfo( curmi->id, controlPath );
		}
	}
	// Update "Mixers" source
	getMixersData();
}

void MixerEngine::clearInternalData(bool removeSources)
{
	for (MixerInfo *mi : qAsConst(m_mixers))
	{
		if ( removeSources )
			removeSource( mi->id );
		delete mi->iface;
		delete mi;
	}
	m_mixers.clear();

	for (ControlInfo *ci : qAsConst(m_controls))
	{
		if ( removeSources )
			removeSource( ci->mixerId + '/' + ci->id );
		delete ci->iface;
		delete ci;
	}
	m_controls.clear();
}

bool MixerEngine::sourceRequestEvent( const QString &name )
{
	if ( name == "Mixers" )
		return getMixersData();
	else if ( name.indexOf("/") == -1 )
		// This request is for mixer
		return getMixerData( name );
	else
		// This request is for control
		return getControlData( name );
}

bool MixerEngine::updateSourceEvent( const QString &name )
{
	return sourceRequestEvent( name );
}

bool MixerEngine::getMixersData()
{
	QStringList mixerIds;
	if ( interface->isServiceRegistered( KMIX_DBUS_SERVICE ) && m_kmix )
	{
		for (const MixerInfo *mi : qAsConst(m_mixers)) mixerIds.append(mi->id);
		/* FIXME: this is used to know whether kmix isn't running or
		 * it can't find any audio device; also it works as a strange 
		 * workaround: without it there is no dataUpdated() call sometimes
		 * when it is updated here */
		setData( "Mixers", "Running", true );
		setData( "Mixers", "Mixers", mixerIds );
		setData( "Mixers", "Current Master Mixer", m_kmix->currentMasterMixer() );
		setData( "Mixers", "Current Master Control", m_kmix->currentMasterControl() );
	}
	else
	{
		setData( "Mixers", "Running", false );
		removeData( "Mixers", "Mixers" );
		removeData( "Mixers", "Current Master Mixer" );
		removeData( "Mixers", "Current Master Control" );
	}
	return true;
}

bool MixerEngine::getMixerData( const QString& source )
{
	// Trying to find this mixer
	MixerInfo *curmi = 0;
	for (MixerInfo *mi : qAsConst(m_mixers))
	{
		if ( mi->id == source )
		{
			curmi = mi;
			break;
		}
	}

	if ( !curmi || !curmi->iface->connection().isConnected() )
		return false;
	// Setting data
	curmi->updateRequired = true;
	QStringList controlIds;
	QStringList controlReadableNames;
	QStringList controlIcons;
	for (const ControlInfo *ci : m_controls.values(curmi->id))
	{
		if ( ci->iface->connection().isConnected() )
		{
			controlIds.append( ci->id );
			controlReadableNames.append( ci->iface->readableName() );
			controlIcons.append( ci->iface->iconName() );
		}
	}

	setData( source, "Opened", curmi->iface->opened() );
	setData( source, "Readable Name", curmi->iface->readableName() );
	setData( source, "Balance", curmi->iface->balance() );
	setData( source, "Controls", controlIds );
	setData( source, "Controls Readable Names", controlReadableNames );
	setData( source, "Controls Icons Names", controlIcons );
	return true;
}

bool MixerEngine::getControlData( const QString &source )
{
	QString mixerId = source.section( '/', 0, 0 );
	QString controlId = source.section( '/', 1 );
	// Trying to find mixer for this control 
	// and monitor for its changes
	for (MixerInfo *mi : qAsConst(m_mixers))
	{
		if ( mi->id == mixerId )
		{
			if ( !mi->connected )
			{
				QDBusConnection::sessionBus().connect( KMIX_DBUS_SERVICE, mi->dbusPath,
						"org.kde.KMix.Mixer", "controlChanged",
						this, SLOT(slotControlChanged()) );
				mi->connected = true;
			}
			break;
		}
	}

	// Trying to find this control
	ControlInfo *curci = 0;
	for (ControlInfo *ci : m_controls.values(mixerId))
	{
		if ( ci->id == controlId ) {
			curci = ci;
			break;
		}
	}

	if ( !curci || !curci->iface->connection().isConnected() )
		return false;
	// Setting data
	curci->updateRequired = true;
	setControlData( curci );
	return true;
}

void MixerEngine::setControlData(ControlInfo* ci)
{
	QString source = ci->mixerId + '/' + ci->id;
	setData( source, "Volume", ci->iface->volume() );
	setData( source, "Mute", ci->iface->mute() );
	setData( source, "Can Be Muted", ci->iface->canMute() );
	setData( source, "Readable Name", ci->iface->readableName() );
	setData( source, "Icon", QIcon::fromTheme(ci->iface->iconName()) );
	setData( source, "Record Source", ci->iface->recordSource() );
	setData( source, "Has Capture Switch", ci->iface->hasCaptureSwitch() );
}


void MixerEngine::slotServiceRegistered( const QString &serviceName)
{
	// Let's give KMix some time to load
	if ( serviceName == KMIX_DBUS_SERVICE )
		QTimer::singleShot(1000, this, &MixerEngine::getInternalData);
}

void MixerEngine::slotServiceUnregistered( const QString &serviceName)
{
	if ( serviceName == KMIX_DBUS_SERVICE )
		clearInternalData(true);
	// Updating 'Mixers' source
	getMixersData();
}

void MixerEngine::slotControlChanged()
{
	// Trying to find mixer from which signal was emitted
	const MixerInfo *curmi = m_mixers.value( message().path(), 0 );
	if (curmi==nullptr) return;
	// Updating all controls that might change
	for (ControlInfo *ci : m_controls.values(curmi->id))
	{
		if (ci->updateRequired) setControlData(ci);
	}
}

void MixerEngine::slotControlsReconfigured()
{
	// Trying to find mixer from which signal was emitted
	const MixerInfo *curmi = m_mixers.value( message().path(), 0 );
	if (curmi==nullptr) return;

	// Updating
	QList<ControlInfo*> controlsForMixer = m_controls.values( curmi->id );
	QStringList controlIds;
	QStringList controlReadableNames;
	QStringList controlIconNames;
	for (ControlInfo *ci : qAsConst(controlsForMixer)) ci->unused = true;
	for (const QString &controlPath : curmi->iface->controls())
	{
		ControlInfo* curci = 0;
		for (ControlInfo *ci : qAsConst(controlsForMixer))
		{
			if ( ci->dbusPath == controlPath )
			{
				curci = ci;
				break;
			}
		}

		// If control not found then we should add a new
		if ( !curci )
			curci = createControlInfo( curmi->id, controlPath );
		curci->unused = false;
		controlIds.append( curci->id );
		controlReadableNames.append( curci->iface->readableName() );
		controlIconNames.append( curci->iface->iconName() );
	}

	// If control is unused then we should remove it
	for (ControlInfo *ci : qAsConst(controlsForMixer))
	{
		if ( ci->unused )
		{
			m_controls.remove( curmi->id, ci );
			delete ci->iface;
			delete ci;
		}
	}
	if ( curmi->updateRequired )
	{
		setData( curmi->id, "Controls", controlIds );
		setData( curmi->id, "Controls Readable Names", controlReadableNames );
		setData( curmi->id, "Controls Icons Names", controlIconNames );
	}
}

void MixerEngine::updateInternalMixersData()
{
	// Some mixer added or removed
	for (MixerInfo *mi : qAsConst(m_mixers)) mi->unused = true;
	for (const QString& mixerPath : m_kmix->mixers())
	{
		MixerInfo* curmi = m_mixers.value( mixerPath, 0 );
		// if mixer was added, we need to add one to m_mixers
		// and add all controls for this mixer to m_controls
		if ( !curmi )
		{
			curmi = createMixerInfo( mixerPath );
			for (const QString &controlPath : curmi->iface->controls())
				createControlInfo( curmi->id, controlPath );
		}
		curmi->unused = false;
	}

	// and if it was removed, we should remove it
	// and remove all controls
	for (MixerInfo *mi : qAsConst(m_mixers))
	{
		if ( mi->unused )
		{
			for (ControlInfo *ci : m_controls.values(mi->id))
			{
				m_controls.remove( mi->id, ci );
				removeSource( ci->mixerId + '/' + ci->id );
				delete ci->iface;
				delete ci;
			}
			m_mixers.remove( mi->dbusPath );
			removeSource( mi->id );
			delete mi->iface;
			delete mi;
		}
	}
}

void MixerEngine::slotMixersChanged()
{
	// Let's give KMix some time to register this mixer on bus and so on
	QTimer::singleShot(1000, this, &MixerEngine::updateInternalMixersData);
}

void MixerEngine::slotMasterChanged()
{
	setData( "Mixers", "Current Master Mixer", m_kmix->currentMasterMixer() );
	setData( "Mixers", "Current Master Control", m_kmix->currentMasterControl() );
}

Plasma::Service* MixerEngine::serviceForSource(const QString& source)
{
	QString mixerId = source.section( '/', 0, 0 );
	QString controlId = source.section( '/', 1 );
	// Trying to find this control
	ControlInfo *curci = 0;
	for (ControlInfo *ci : m_controls.values(mixerId))
	{
		if ( ci->id == controlId ) {
			curci = ci;
			break;
		}
	}

	if (curci==nullptr) return Plasma::DataEngine::serviceForSource(source);
	return (new MixerService(this, curci->iface));
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(mixer, MixerEngine, "plasma-dataengine-mixer.json")

#include "mixerengine.moc"
