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
   MixerToolBox::initMixer(m_multiDriverMode, m_backendFilter, true);

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
   const Mixer *mixerMasterCard = Mixer::getGlobalMasterMixer();
   if (mixerMasterCard!=nullptr) Settings::setMasterMixer(mixerMasterCard->id());
   shared_ptr<MixDevice> mdMaster = Mixer::getGlobalMasterMD();
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
    Mixer::setGlobalMaster(mixerMasterCard, masterDev, true);
    QString mixerIgnoreExpression = Settings::mixerIgnoreExpression();
    if (!mixerIgnoreExpression.isEmpty()) MixerToolBox::setMixerIgnoreExpression(mixerIgnoreExpression);

    m_backendFilter = Settings::backends();
    MixerToolBox::setMixerIgnoreExpression(mixerIgnoreExpression);
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


void KMixD::unplugged(const QString &udi)
{
    qCDebug(KMIX_LOG) << "udi" << udi;

//     qCDebug(KMIX_LOG) << "Unplugged: udi=" <<udi << "\n";
    for (int i=0; i<Mixer::mixers().count(); ++i) {
        Mixer *mixer = (Mixer::mixers())[i];
//         qCDebug(KMIX_LOG) << "Try Match with:" << mixer->udi() << "\n";
        if (mixer->udi() == udi ) {
            qCDebug(KMIX_LOG) << "Unplugged Match: Removing udi=" <<udi << "\n";
            //KMixToolBox::notification("MasterFallback", "aaa");
            bool globalMasterMixerDestroyed = ( mixer == Mixer::getGlobalMasterMixer() );

            MixerToolBox::removeMixer(mixer);
            // Check whether the Global Master disappeared, and select a new one if necessary
            shared_ptr<MixDevice> md = Mixer::getGlobalMasterMD();
            if ( globalMasterMixerDestroyed || md.get() == 0 ) {
                // We don't know what the global master should be now.
                // So lets play stupid, and just select the recommended master of the first device
                if ( Mixer::mixers().count() > 0 ) {
                	shared_ptr<MixDevice> master = ((Mixer::mixers())[0])->getLocalMasterMD();
                    if ( master.get() != 0 ) {
                        QString localMaster = master->id();
                        Mixer::setGlobalMaster( ((Mixer::mixers())[0])->id(), localMaster, false);

                        QString text;
                        text = i18n("The soundcard containing the master device was unplugged. Changing to control %1 on card %2.",
                                master->readableName(),
                                ((Mixer::mixers())[0])->readableName()
                        );
//                        KMixToolBox::notification("MasterFallback", text);
                    }
                }
            }
            if ( Mixer::mixers().count() == 0 ) {
                QString text;
                text = i18n("The last soundcard was unplugged.");
//                KMixToolBox::notification("MasterFallback", text);
            }
            break;
        }
    }

}


#include "kmixd.moc"
