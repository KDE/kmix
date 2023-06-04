/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 1996-2000 Christian Esken <esken@kde.org>
 * Copyright 2000-2003 Christian Esken <esken@kde.org>, Stefan Schimanski <1Stein@gmx.de>
 * Copyright 2002-2007 Christian Esken <esken@kde.org>, Helio Chissini de Castro <helio@conectiva.com.br>
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

#include "kmixd.h"

#include <kconfig.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kpluginloader.h> 

// KMix
#include "core/mixertoolbox.h"
#include "core/kmixdevicemanager.h"
#include "core/mixer.h"
#include "settings.h"


K_PLUGIN_FACTORY_WITH_JSON(KMixDFactory,
                           "kmixd.json",
                           registerPlugin<KMixD>();)


/* KMixD
 * Constructs a mixer window (KMix main window)
 */
KMixD::KMixD(QObject* parent, const QList<QVariant>&) :
   KDEDModule(parent),
   m_multiDriverMode (false) // -<- I never-ever want the multi-drivermode to be activated by accident
{
    setObjectName( QStringLiteral("KMixD" ));
	qCDebug(KMIX_LOG) << "kmixd: Triggering delayed initialization";
	QTimer::singleShot( 3000, this, SLOT(delayedInitialization()));
}

/**
 * This is called by a singleShot timer. Reason is that KMixD seems to "collide" with other applications
 * on DBUS, likely creating deadlocks. See Bug 317926 for that. By artificially delaying initialization,
 * KMixD gets hopefully out of the way of the other applications, avoiding these deadlocks. A small delay
 * should also not harm too much, as apps using the Mixer DBUS calls on KMixD must be prepared that the
 * interface is not available right when they start.
 */
void KMixD::delayedInitialization()
{
    qCDebug(KMIX_LOG) << "Delayed initialization running now";
   //initActions(); // init actions first, so we can use them in the loadConfig() already
   loadConfig(); // Load config before initMixer(), e.g. due to "MultiDriver" keyword
   MixerToolBox::initMixer(true);

   KMixDeviceManager *theKMixDeviceManager = KMixDeviceManager::instance();
   connect(theKMixDeviceManager, &KMixDeviceManager::plugged, this, &KMixD::plugged);
   connect(theKMixDeviceManager, &KMixDeviceManager::unplugged, this, &KMixD::unplugged);
   theKMixDeviceManager->initHotplug();

   qCDebug(KMIX_LOG) << "Delayed initialization done";
}


KMixD::~KMixD()
{
   MixerToolBox::deinitMixer();
}



void KMixD::saveConfig()
{
   qCDebug(KMIX_LOG) << "About to save config";
   saveBaseConfig();
  // saveVolumes(); // -<- removed from kmixd, as it is possibly a bad idea if both kmix and kmixd write the same config "kmixctrlrc"
#ifdef __GNUC_
#warn We must Sync here, or we will lose configuration data. The reson for that is unknown.
#endif

   qCDebug(KMIX_LOG) << "Saved config ... now syncing explicitly";
   Settings::self()->save();
   qCDebug(KMIX_LOG) << "Saved config ... sync finished";
}

void KMixD::saveBaseConfig()
{
   qCDebug(KMIX_LOG) << "About to save config (Base)";

   Settings::setConfigVersion(KMIX_CONFIG_VERSION);
   const Mixer *mixerMasterCard = MixerToolBox::getGlobalMasterMixer();
   if (mixerMasterCard!=nullptr) Settings::setMasterMixer(mixerMasterCard->id());
   shared_ptr<MixDevice> mdMaster = MixerToolBox::getGlobalMasterMD();
   if (mdMaster) Settings::setMasterMixerDevice(mdMaster->id());
   Settings::setMixerIgnoreExpression(MixerToolBox::mixerIgnoreExpression());
   qCDebug(KMIX_LOG) << "Config (Base) saving done";
}

void KMixD::loadConfig()
{
   loadBaseConfig();
}

void KMixD::loadBaseConfig()
{
    m_multiDriverMode = Settings::multiDriver();
    QString mixerMasterCard = Settings::masterMixer();
    QString masterDev = Settings::masterMixerDevice();
    MixerToolBox::setGlobalMaster(mixerMasterCard, masterDev, true);

    const QString mixerIgnoreExpression = Settings::mixerIgnoreExpression();
    if (!mixerIgnoreExpression.isEmpty()) MixerToolBox::setMixerIgnoreExpression(mixerIgnoreExpression);
    const QStringList allowedBackends = Settings::backends();
    if (!allowedBackends.isEmpty()) MixerToolBox::setAllowedBackends(allowedBackends);
}


void KMixD::plugged(const char *driverName, const QString &udi, int dev)
{
    qCDebug(KMIX_LOG) << "dev" << dev << "driver" << driverName << "udi" << udi;

    Mixer *mixer = new Mixer(QString::fromLocal8Bit(driverName), dev);
    if (mixer!=nullptr)
    {
        qCDebug(KMIX_LOG) << "adding mixer" << mixer->id() << mixer->readableName();
        MixerToolBox::possiblyAddMixer(mixer);
    }
}


// This is much the same as KMixWindow::unplugged()
// except that there is no GUI.

void KMixD::unplugged(const QString &udi)
{
	qCDebug(KMIX_LOG) << "UDI" << udi;

	// This assumes that there can be at most one mixer in the list
	// with the given UDI.
	Mixer *unpluggedMixer = nullptr;
	for (Mixer *mixer : qAsConst(MixerToolBox::mixers()))
	{
		if (mixer->hotplugId()==udi)
		{
			unpluggedMixer = mixer;
			break;
		}
	}

	if (unpluggedMixer==nullptr)
	{
		qCDebug(KMIX_LOG) << "No mixer with that UDI";
		return;
	}

	qCDebug(KMIX_LOG) << "Removing mixer";
	const bool globalMasterMixerDestroyed = (unpluggedMixer==MixerToolBox::getGlobalMasterMixer());

	// Remove the mixer from the known list
	MixerToolBox::removeMixer(unpluggedMixer);

	// Check whether the Global Master disappeared,
	// and select a new one if necessary
	shared_ptr<MixDevice> md = MixerToolBox::getGlobalMasterMD();
	if (globalMasterMixerDestroyed || md==nullptr)
	{
		const QList<Mixer *> mixers = MixerToolBox::mixers();
		if (!mixers.isEmpty())
		{
			shared_ptr<MixDevice> master = mixers.first()->getLocalMasterMD();
			if (master!=nullptr)
			{
				Mixer *mixer = mixers.first();
				QString localMaster = master->id();
				MixerToolBox::setGlobalMaster(mixer->id(), localMaster, false);
			}
		}
	}
}


#include "kmixd.moc"
