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


#include <QWidget>
#include <QString>

//#include <kdebug.h>
#include <kglobalaccel.h>
#include <klocale.h>
#include <knotification.h>

#include "guiprofile.h"
#include "mdwslider.h"
#include "mixdevicewidget.h"
#include "mixdevice.h"
#include "mixer.h"
#include "viewbase.h"

#include "kmixtoolbox.h"

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

void KMixToolBox::loadView(ViewBase *view, KConfig *config)
{
   QString grp = "View.";
   grp += view->objectName();
   KConfigGroup cg = config->group( grp );
   kDebug(67100) << "KMixToolBox::loadView() grp=" << grp.toAscii();

   static char guiComplexity[3][20] = { "simple", "extended", "all" };
   for ( int tries = 0; tries < 3; tries++ )
   {
   bool atLeastOneControlIsShown = false;
   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;
         QString devgrp;
         devgrp.sprintf( "%s.%s.%s", grp.toAscii().data(), view->id().toAscii().data(), mdw->mixDevice()->id().toAscii().data() );
         KConfigGroup devcg  = config->group( devgrp );

         if ( mdw->inherits("MDWSlider") )
         {
            // only sliders have the ability to split apart in mutliple channels
            bool splitChannels = devcg.readEntry("Split", false);
            mdw->setStereoLinked( !splitChannels );
         }

         bool mdwEnabled = false;
         if ( devcg.hasKey("Show") ) 
         {
            mdwEnabled = ( true == devcg.readEntry("Show", true) );
            //kDebug(67100) << "KMixToolBox::loadView() for" << devgrp << "from config-file: mdwEnabled==" << mdwEnabled;
         }
         else
         {
            // if not configured in config file, use the default from the profile
             GUIProfile::ControlSet cset = (view->guiProfile()->_controls);
             for ( std::vector<ProfControl*>::const_iterator it = cset.begin(); it != cset.end(); ++it)
             {
                ProfControl* pControl = *it;
                QRegExp idRegExp(pControl->id);
                //kDebug(67100) << "KMixToolBox::loadView() try match " << (*pControl).id << " for " << mdw->mixDevice()->id();
                if ( mdw->mixDevice()->id().contains(idRegExp) ) {
                   if ( pControl->show == guiComplexity[tries] )
                   {
                      mdwEnabled = true;
                      atLeastOneControlIsShown = true;
                      //kDebug(67100) << "KMixToolBox::loadView() for" << devgrp << "from profile: mdwEnabled==" << mdwEnabled;
                   }
                   break;
                }
             } // iterate over all ProfControl entries
         }
         //mdw->setEnabled(mdwEnabled);  // I have no idea why dialogselectmaster works with "enabled" instead of "visible"
         if (!mdwEnabled) { mdw->hide(); } else { mdw->show(); }

      } // inherits MixDeviceWidget
   } // for all MDW's
   if ( atLeastOneControlIsShown ) {
      break;   // If there were controls in this complexity level, don't try more
   }
   } // for try = 0 ... 1         //kDebug(67100) << "KMixToolBox::loadView() for" << devgrp << "FINAL: mdwEnabled==" << mdwEnabled;
}

void KMixToolBox::loadKeys(ViewBase *view, KConfig */*config*/)
// !!! this must be moved out of the views into the kmixd
{
   kDebug(67100) << "KMixToolBox::loadKeys()";
   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;
         KGlobalAccel *keys = KGlobalAccel::self();
         if ( keys )
         {
            QString devgrpkeys;
            devgrpkeys.sprintf( "Keys.%s.%s", view->id().toAscii().data(), mdw->mixDevice()->id().toAscii().data() );
            //kDebug(67100) << "KMixToolBox::loadKeys() load Keys " << devgrpkeys;

            // please see KMixToolBox::saveKeys() for some rambling about saving/loading Keys

            //Note to maintainer: this should be correct [you cannot choose the group anymore!],
            //and if you really need groups, I recommend prepending group names to the action
            //names. -- ahartmetz

            //keys->setConfigGroup(devgrpkeys);
#ifdef __GNUC__
#warning port me - it is probably safe to just remove this line as *global* shortcut setttings
#warning are now saved and loaded automatically by default.
#endif
            //keys->readSettings();
         } // MDW has keys
      } // is a MixDeviceWidget
   } // for all widgets
}

/*
 * Saves the View configuration
 */
void KMixToolBox::saveView(ViewBase *view, KConfig *config)
{
   QString grp = "View.";
   grp += view->objectName();
   KConfigGroup cg = config->group( grp );
   kDebug(67100) << "KMixToolBox::saveView() grp=" << grp.toAscii();

   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;

         //kDebug(67100) << "  grp=" << grp.toAscii();
         //kDebug(67100) << "  mixer=" << view->id().toAscii();
         //kDebug(67100) << "  mdwPK=" << mdw->mixDevice()->id().toAscii();

         QString devgrp;
         devgrp.sprintf( "%s.%s.%s", grp.toAscii().data(), view->id().toAscii().data(), mdw->mixDevice()->id().toAscii().data() );
         KConfigGroup devcg = config->group( devgrp );

         if ( mdw->inherits("MDWSlider") )
         {
            // only sliders have the ability to split apart in mutliple channels
            devcg.writeEntry( "Split", ! mdw->isStereoLinked() );
         }
         devcg.writeEntry( "Show" , mdw->isVisibleTo(view) );
      } // inherits MixDeviceWidget
   } // for all MDW's
}


// Save key bindings
void KMixToolBox::saveKeys(ViewBase *view, KConfig */*config*/)
// !!! this must be moved out of the views into the kmixd
{
   /*
       Implementation hint: Conceptually keys SHOULD be bound to the actual hardware, and not
       to one GUI representation. Both work, but it COULD confuse users, if we have multiple
       GUI representations (e.g. "Dock Icon" and "Main Window").
       If you think about this aspect more deeply, you will find out that this is the case already
       today with "kmixapplet" and "kmix main application". It would really nice to rework this.
    */
   kDebug(67100) << "KMixToolBox::saveKeys()";
   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;
         KGlobalAccel *keys = KGlobalAccel::self();
         if ( keys )
         {
            QString devgrpkeys;
            devgrpkeys.sprintf( "Keys.%s.%s", view->id().toAscii().data(), mdw->mixDevice()->id().toAscii().data() );
            //kDebug(67100) << "KMixToolBox::saveKeys() : " << devgrpkeys;

            //See note in loadKeys! -- ahartmetz
            //keys->setConfigGroup(devgrpkeys);
#ifdef __GNUC__
#warning port me - it is probably safe to just remove this line as *global* shortcut setttings
#warning are now saved and loaded automatically by default.
#endif
            //keys->writeSettings();
         } // MDW has keys
      } // is a MixDeviceWidget
   } // for all widgets
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
        QObject::connect(notification, SIGNAL(activated(unsigned int)), receiver, actionSlot);
    }
    notification->sendEvent();
}
