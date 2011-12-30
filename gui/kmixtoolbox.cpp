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

#include <QWidget>
#include <QString>

//#include <kdebug.h>
#include <kglobalaccel.h>
#include <klocale.h>
#include <knotification.h>

#include "gui/guiprofile.h"
#include "mdwslider.h"
#include "gui/mixdevicewidget.h"
#include "core/mixdevice.h"
#include "core/mixer.h"
#include "viewbase.h"

// TODO KMixToolbox is rather superfluous today, as there is no "KMix Applet" any more, and it was probably always bad style.
//      I only have to think what to do with KMixToolBox::notification()

/***********************************************************************************
 KMixToolbox contains several GUI relevant methods that are shared between the 
 KMix Main Program, and the KMix Applet.
 kmixctrl - as not non-GUI application - does NOT link to KMixToolBox.

 This means: Shared GUI stuff goes into the KMixToolBox class , non-GUI stuff goes
 into the MixerToolBox class.
 ***********************************************************************************/
void KMixToolBox::setIcons(QList<QWidget *> &mdws, bool on ) {
   for (int i=0; i < mdws.count(); ++i ){
      QWidget *mdw = mdws[i];
      if ( mdw->inherits("MixDeviceWidget") ) { // -<- play safe here
         static_cast<MixDeviceWidget*>(mdw)->setIcons( on );
      }
   }
}

void KMixToolBox::setLabels(QList<QWidget *> &mdws, bool on ) {
   for (int i=0; i < mdws.count(); ++i ){
      QWidget *mdw = mdws[i];
      if ( mdw->inherits("MixDeviceWidget") ) { // -<- play safe here
         static_cast<MixDeviceWidget*>(mdw)->setLabeled( on );
      }
   }
}

void KMixToolBox::setTicks(QList<QWidget *> &mdws, bool on ) {
   for (int i=0; i < mdws.count(); ++i ){
      QWidget *mdw = mdws[i];
      if ( mdw->inherits("MixDeviceWidget") ) { // -<- play safe here
         static_cast<MixDeviceWidget*>(mdw)->setTicks( on );
      }
   }
}

void KMixToolBox::notification(const char *notificationName, const QString &text,
                                const QStringList &actions, QObject *receiver,
                                const char *actionSlot)
{
    KNotification *notification = new KNotification(notificationName);
    //notification->setComponentData(componentData());
    notification->setText(text);
    //notification->setPixmap(...);
    notification->addContext(QLatin1String("Application"), KGlobal::mainComponent().componentName());
    if (!actions.isEmpty() && receiver && actionSlot) {
        notification->setActions(actions);
        QObject::connect(notification, SIGNAL(activated(uint)), receiver, actionSlot);
    }
    notification->sendEvent();
}
