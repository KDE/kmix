/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2006-2007 Christian Esken <esken@kde.org>
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

#include "core/kmixdevicemanager.h"

#ifdef __GNUC__
#warning KMix KF5 build does not support hotplugging yet
#endif

#include <iostream>

#include <QRegExp>
#include <QString>

// #include <solid/device.h>
// #include <solid/devicenotifier.h>
// #include <solid/audiointerface.h>

KMixDeviceManager* KMixDeviceManager::s_KMixDeviceManager = 0;

KMixDeviceManager::KMixDeviceManager()
{
}

KMixDeviceManager::~KMixDeviceManager()
{
}

KMixDeviceManager* KMixDeviceManager::instance()
{
    if ( s_KMixDeviceManager == 0 ) {
        s_KMixDeviceManager = new KMixDeviceManager();
    }
    return s_KMixDeviceManager;
}

void KMixDeviceManager::initHotplug()
{
//     connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)), SLOT(pluggedSlot(QString)) );
//     connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)), SLOT(unpluggedSlot(QString)) );
}

QString KMixDeviceManager::getUDI_ALSA(int num)
{
	return (QString("hw%1").arg(num));

#if 0
        QList<Solid::Device> dl = Solid::Device::listFromType(Solid::DeviceInterface::AudioInterface);

	QString numString;
	numString.setNum(num);
	bool found = false;
	QString udi;
	QString devHandle;
	foreach ( const Solid::Device &device, dl )
	{
	//        std::cout << "Coldplug udi = '" << device.udi().toUtf8().data() << "'\n";
		// LEAK audiohw leaks, but Solid does not document whether it is its own or "my" Object.
		const Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
		if (audiohw != 0)
		{
			if (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))
			{
				switch (audiohw->driver())
				{
					case Solid::AudioInterface::Alsa:
						devHandle = audiohw->driverHandle().toList().first().toString();
						if ( numString == devHandle )
						{
							found = true;
							udi = device.udi();
						}
						break;
					default:
						break;
				} // driver type
			} // is an audio control

			// If I delete audiohw, kmix crashes. If I do not, there is a definite leak according to valgrind:
//			==24561== 4,958 (1,200 direct, 3,758 indirect) bytes in 25 blocks are definitely lost in loss record 1,882 of 1,918
//			==24561==    at 0x4C27D49: operator new(unsigned long) (in /usr/lib64/valgrind/vgpreload_memcheck-amd64-linux.so)
//			==24561==    by 0x5665462: ??? (in /usr/lib64/libsolid.so.4.11.3)
//			==24561==    by 0x565EC57: ??? (in /usr/lib64/libsolid.so.4.11.3)
//			==24561==    by 0x563610B: Solid::Device::asDeviceInterface(Solid::DeviceInterface::Type const&) const (in /usr/lib64/libsolid.so.4.11.3)
//			==24561==    by 0x4EB7DA6: Solid::AudioInterface const* Solid::Device::as<Solid::AudioInterface>() const (device.h:254)
//			==24561==    by 0x4EB6F6F: KMixDeviceManager::getUDI_ALSA(int) (kmixdevicemanager.cpp:70)
//			==24561==    by 0x4E6C809: Mixer_ALSA::open() (mixer_alsa9.cpp:139)
//			==24561==    by 0x4E6334E: Mixer_Backend::openIfValid() (mixer_backend.cpp:84)
//			==24561==    by 0x4EBD4F9: Mixer::openIfValid(int) (mixer.cpp:268)
//			==24561==    by 0x4EB5F10: MixerToolBox::possiblyAddMixer(Mixer*) (mixertoolbox.cpp:310)
//			==24561==    by 0x4EB57AE: MixerToolBox::initMixerInternal(MixerToolBox::MultiDriverMode, QList<QString>, QString&) (mixertoolbox.cpp:165)
//			==24561==    by 0x4EB52B6: MixerToolBox::initMixer(MixerToolBox::MultiDriverMode, QList<QString>, QString&) (mixertoolbox.cpp:85)
//			==24561==    by 0x4EB5267: MixerToolBox::initMixer(bool, QList<QString>, QString&) (mixertoolbox.cpp:80)
//			==24561==    by 0x4E7ED75: KMixWindow::KMixWindow(bool) (kmix.cpp:96)
//			==24561==    by 0x4E8A7CF: KMixApp::newInstance() (KMixApp.cpp:112)
//			==24561==    by 0x5B30A7E: KUniqueApplication::Private::_k_newInstanceNoFork() (in /usr/lib64/libkdeui.so.5.11.3)
//			==24561==    by 0x774A11D: QObject::event(QEvent*) (in /usr/lib64/libQtCore.so.4.8.5)
//			==24561==    by 0x61338A2: QApplication::event(QEvent*) (in /usr/lib64/libQtGui.so.4.8.5)
//			==24561==    by 0x612E8AB: QApplicationPrivate::notify_helper(QObject*, QEvent*) (in /usr/lib64/libQtGui.so.4.8.5)
//			==24561==    by 0x6134E6F: QApplication::notify(QObject*, QEvent*) (in /usr/lib64/libQtGui.so.4.8.5)

			// The data seems to be "static" device info from Solid, so I will leave it alone.
//				delete audiohw;
		}
		if (found)
		{
			break;
		}
	} // foreach
	return udi;
#endif // 0
}

QString KMixDeviceManager::getUDI_OSS(const QString& devname)
{
	QString udi(devname);
	return udi;

#if 0
    QList<Solid::Device> dl = Solid::Device::listFromType(Solid::DeviceInterface::AudioInterface);

    bool found = false;
    QString udi;
    QString devHandle;
    foreach ( const Solid::Device &device, dl )
    {
//        std::cout << "Coldplug udi = '" << device.udi().toUtf8().data() << "'\n";
        const Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
        if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
            switch (audiohw->driver()) {
                case Solid::AudioInterface::OpenSoundSystem:
                    devHandle = audiohw->driverHandle().toString();
//                    std::cout << ">>> Coldplugged OSS ='" <<  devHandle.toUtf8().data() << "'\n";
                    if ( devname == devHandle ) {
                        found = true;
//                        std::cout << ">>> Match!!! Coldplugged OSS ='" <<  devHandle.toUtf8().data() << "'\n";
                        udi = device.udi();
                    }
                     break;
                default:
                    break;
            } // driver type
        } // is an audio control
        if ( found) break;
    } // foreach
    return udi;
#endif // 0
}


void KMixDeviceManager::pluggedSlot(const QString& udi)
{
    return;

#if 0
//   std::cout << "Plugged udi='" <<  udi.toUtf8().data() << "'\n";
   Solid::Device device(udi);
   Solid::AudioInterface *audiohw = device.as<Solid::AudioInterface>();
   if (audiohw && (audiohw->deviceType() & ( Solid::AudioInterface::AudioControl))) {
       QString dev;
       QRegExp devExpr( QLatin1String( "^\\D+(\\d+)$" ));
        switch (audiohw->driver()) {
           case Solid::AudioInterface::Alsa:
               if ( _hotpluggingBackend == "ALSA" || _hotpluggingBackend == "*" ) {
                    dev = audiohw->driverHandle().toList().first().toString();
                    emit plugged("ALSA", udi, dev);
               }
               break;
           case Solid::AudioInterface::OpenSoundSystem:
                if ( _hotpluggingBackend == "OSS" || _hotpluggingBackend == "*" ) {
                    dev = audiohw->driverHandle().toString();
                    if ( devExpr.indexIn(dev) > -1 ) {
                        dev = devExpr.cap(1); // Get device number from device name (e.g "/dev/mixer1" or "/dev/sound/mixer2")
                    }
                    else {
                        dev = '0'; // "/dev/mixer" or "/dev/sound/mixer"
                    }
                    emit plugged("OSS", udi, dev);
                }
                break;
           default:
               qCCritical(KMIX_LOG) <<  "Plugged UNKNOWN Audio device (ignored)";
               break;
       }
    }
#endif // 0
}


void KMixDeviceManager::unpluggedSlot(const QString& udi) {
//    std::cout << "Unplugged udi='" <<  udi.toUtf8().data() << "'\n";
//    Solid::Device device(udi);
    // At this point the device has already been unplugged by the user. Solid doesn't know anything about the
    // device except the UDI (not even device.as<Solid::AudioInterface>() is possible). Thus I'll forward any
    // unplugging action (could e.g. also be HID or mass storage). The receiver of the signal has to deal with it,
    // but a simple UDI matching is enough.
    emit unplugged(udi);

}



