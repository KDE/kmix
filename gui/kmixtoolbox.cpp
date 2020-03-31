/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2004 Christian Esken <esken@kde.org>
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

#include "gui/kmixtoolbox.h"

#include <QCoreApplication>

#include <knotification.h>
#include <kmessagewidget.h>
#include <klocalizedstring.h>

#include "gui/mixdevicewidget.h"

/***********************************************************************************
 KMixToolbox contains several GUI utility functions that are only used by the 
 KMix main program.  kded_kmix and kmixctrl - as non-GUI applications - do not
 use or link to KMixToolBox.

 This means that shared GUI stuff goes into KMixToolBox here, non-GUI stuff goes
 into the MixerToolBox class.
 ***********************************************************************************/

void KMixToolBox::setIcons(QList<QWidget *> &mdws, bool on)
{
    for (QWidget *w : mdws)
    {
        MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget *>(w);
        if (mdw!=nullptr) mdw->setIcons(on);
    }
}


void KMixToolBox::setLabels(QList<QWidget *> &mdws, bool on)
{
    for (QWidget *w : mdws)
    {
        MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget *>(w);
        if (mdw!=nullptr) mdw->setLabeled(on);
    }
}


void KMixToolBox::setTicks(QList<QWidget *> &mdws, bool on)
{
    for (QWidget *w : mdws)
    {
        MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget *>(w);
        if (mdw!=nullptr) mdw->setTicks(on);
    }
}


void KMixToolBox::notification(const char *notificationName, const QString &text)
{
    KNotification *notification = new KNotification(notificationName);
    notification->setText(text);
    notification->setIconName(QLatin1String("kmix"));
    notification->addContext(QLatin1String("Application"), QCoreApplication::applicationName());
    notification->sendEvent();
    // Otherwise there will be a memory leak (although the notification is
    // rarely shown).  Assuming that it is safe to delete here.
    notification->deleteLater();
}


QWidget *KMixToolBox::noDevicesWarningWidget(QWidget *parent)
{
    KMessageWidget *mw = new KMessageWidget(noDevicesWarningString(), parent);
    mw->setIcon(QIcon::fromTheme("dialog-warning"));
    mw->setMessageType(KMessageWidget::Warning);
    mw->setCloseButtonVisible(false);
    mw->setWordWrap(true);
    return (mw);
}


QString KMixToolBox::noDevicesWarningString()
{
    return (i18n("No sound devices are installed or are currently available."));
}
