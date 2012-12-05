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

#ifndef MIXER_ENGINE_H
#define MIXER_ENGINE_H

#include <Plasma/DataEngine>
#include <Plasma/Service>

#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDBusContext>
#include <QHash>
#include <QMultiHash>
#include <QString>

class OrgKdeKMixMixSetInterface;
class OrgKdeKMixMixerInterface;
class OrgKdeKMixControlInterface;

struct ControlInfo {
	QString mixerId;
	QString id;
	QString dbusPath;
	bool unused;
	bool updateRequired;
	OrgKdeKMixControlInterface *iface;
};

struct MixerInfo {
	QString id;
	QString dbusPath;
	bool unused;
	bool updateRequired;
	bool connected;
	OrgKdeKMixMixerInterface *iface;
};

class MixerEngine : public Plasma::DataEngine,
					protected QDBusContext
{
	Q_OBJECT

public:
	MixerEngine(QObject* parent, const QVariantList& args);
	~MixerEngine();

	QStringList sources() const;

	void init();
	Plasma::Service* serviceForSource(const QString& source);
protected:
	bool sourceRequestEvent( const QString &name );
	bool updateSourceEvent( const QString &name );
private:
	static const QString KMIX_DBUS_SERVICE;
	static const QString KMIX_DBUS_PATH;
	QDBusConnectionInterface *interface;
	QDBusServiceWatcher *watcher;
	OrgKdeKMixMixSetInterface *m_kmix;
	// Keys are mixers DBus paths
	QHash<QString,MixerInfo*> m_mixers;
	// Keys are mixerIds for control
	QMultiHash<QString,ControlInfo*> m_controls;

	MixerInfo* createMixerInfo( const QString& dbusPath );
	ControlInfo* createControlInfo( const QString& mixerId, const QString& dbusPath );

	void clearInternalData(bool removeSources);
	bool getMixersData();
	bool getMixerData( const QString& source );
	bool getControlData( const QString& source );
	void setControlData( ControlInfo *ci );
private slots:
	void getInternalData();
	void updateInternalMixersData();
	void slotServiceRegistered( const QString &serviceName );
	void slotServiceUnregistered( const QString &serviceName );

	void slotMixersChanged();
	void slotMasterChanged();
	void slotControlChanged();
	void slotControlsReconfigured();
};

#endif /* MIXER_ENGINE_H */
