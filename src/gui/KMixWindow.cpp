/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 1996-2011 Christian Esken <esken@kde.org>
 * Copyright 2000-2003 Christian Esken <esken@kde.org>, Stefan Schimanski <1Stein@gmx.de>
 * Copyright 2002-2007 Christian Esken <esken@kde.org>, Helio Chissini de Castro <helio@conectiva.com.br>
 * Copyright 2012 Trever Fischer <tdfischer@fedoraproject.org>
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

#include "KMixWindow.h"

// include files for QT
#include <QtGui/QHBoxLayout>
#include <QtGui/QTabWidget>

// include files for KDE
#include <KDE/KActionCollection>
#include <KDE/KProcess>
#include <KDE/KXMLGUIFactory>
#include <KDE/KAction>
#include <KDE/KLocale>
#include <KDE/KApplication>

// KMix
#include "kmixd_interface.h"
#include "controlgroup_interface.h"

#include "ControlGroupTab.h"

const QString KMIX_DBUS_SERVICE = "org.kde.kmixd";
const QString KMIX_DBUS_PATH = "/KMixD";

/* KMixWindow
 * Constructs a mixer window (KMix main window)
 */
KMixWindow::KMixWindow(QWidget* parent)
    : KXmlGuiWindow(parent, Qt::WindowFlags( KDE_DEFAULT_WINDOWFLAGS | Qt::WindowContextHelpButtonHint) )
{
    // disable delete-on-close because KMix might just sit in the background waiting for cards to be plugged in
    setAttribute(Qt::WA_DeleteOnClose, false);
    m_daemon = new org::kde::KMix::KMixD(KMIX_DBUS_SERVICE, KMIX_DBUS_PATH, QDBusConnection::sessionBus(), this);
    initActions();
    createGUI( QLatin1String( "kmixui.rc" ) );
    show();
    QTabWidget *mainWidget = new QTabWidget(this);
    setCentralWidget(mainWidget);
    foreach(const QString &groupName, m_daemon->mixerGroups()) {
        org::kde::KMix::ControlGroup *group = new org::kde::KMix::ControlGroup(KMIX_DBUS_SERVICE, groupName, QDBusConnection::sessionBus(), this);
        QWidget *groupWidget = new ControlGroupTab(group, mainWidget);
        mainWidget->addTab(groupWidget, group->displayName());
    }
}

KMixWindow::~KMixWindow()
{
}

void KMixWindow::initActions()
{
    KStandardAction::quit(kapp, SLOT(quit()), actionCollection());
    KStandardAction::keyBindings( guiFactory(), SLOT(configureShortcuts()), actionCollection());

    KAction* action = actionCollection()->addAction( "launch_kdesoundsetup" );
    action->setText( i18n( "Audio Setup" ) );
    connect(action, SIGNAL(triggered(bool)), SLOT(launchPhononConfig()));
}

void KMixWindow::launchPhononConfig()
{
    QStringList args;
    args << "kcmshell4" << "kcm_phonon";
    KProcess::startDetached(args);
}

#include "KMixWindow.moc"
