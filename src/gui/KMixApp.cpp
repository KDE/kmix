/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2000 Stefan Schimanski <schimmi@kde.org>
 * Copyright (C) 2001 Preston Brown <pbrown@kde.org>
 * Copyright (C) 2013 Trever Fischer <tdfischer@kde.org>
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

#include "KMixApp.h"
#include "KMixOSD.h"
#include "KMixWindow.h"
#include <kdebug.h>
#include <KDE/KStatusNotifierItem>
#include "kmixd_interface.h"

org::kde::KMix::KMixD *KMixApp::s_daemon = 0;

KMixApp::KMixApp()
    : KUniqueApplication(), m_kmix( 0 )
{
    // We must disable QuitOnLastWindowClosed. Rationale:
    // 1) The normal state of KMix is to only have the dock icon shown.
    // 2a) The dock icon gets reconstructed, whenever a soundcard is hotplugged or unplugged.
    // 2b) The dock icon gets reconstructed, when the user selects a new master.
    // 3) During the reconstruction, it can easily happen that no window is present => KMix would quit
    // => disable QuitOnLastWindowClosed
    setQuitOnLastWindowClosed ( false );
}


KMixApp::~KMixApp()
{
   delete m_kmix;
}


int
KMixApp::newInstance()
{
    if ( m_kmix ) {
        m_kmix->show();
    } else {
        m_kmix = new KMixWindow(NULL);
        m_osd = new KMixOSD(NULL);
        connect(m_kmix, SIGNAL(disableOSD()), m_osd, SLOT(disable()));
        connect(m_kmix, SIGNAL(enableOSD()), m_osd, SLOT(enable()));
        m_icon = new KStatusNotifierItem("kmix");
        m_icon->setAssociatedWidget(m_kmix);
        m_icon->setCategory(KStatusNotifierItem::Hardware);
        m_icon->setIconByName("kmix");
        m_icon->setStandardActionsEnabled(true);
        m_icon->setStatus(KStatusNotifierItem::Passive);
        m_icon->setTitle("KMix");
        m_icon->setToolTip("kmix", "KMix", "Volume");

        m_kmix->show();
    }
	return 0;
}

org::kde::KMix::KMixD *KMixApp::daemon()
{
    Q_ASSERT(qApp);
    if (s_daemon == 0) {
        s_daemon = new org::kde::KMix::KMixD(KMIX_DBUS_SERVICE, KMIX_DBUS_PATH, QDBusConnection::sessionBus(), qApp);
    }
    return s_daemon;
}
