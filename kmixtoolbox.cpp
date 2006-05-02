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


#include <qcolor.h>
#include <qwidget.h>
#include <qstring.h>

//#include <kdebug.h>
#include <kglobalaccel.h>
#include <klocale.h>

#include "mdwslider.h"
#include "mdwswitch.h"
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
   grp += view->name();
   config->setGroup( grp );
   kDebug(67100) << "KMixToolBox::loadView() grp=" << grp.ascii() << endl;

   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;
         QString devgrp;
         devgrp.sprintf( "%s.%s.%s", grp.ascii(), view->getMixer()->id().ascii(), mdw->mixDevice()->id().ascii() );
         config->setGroup( devgrp );

         if ( mdw->inherits("MDWSlider") )
         {
            // only sliders have the ability to split apart in mutliple channels
            bool splitChannels = config->readEntry("Split", false);
            mdw->setStereoLinked( !splitChannels );
         }
         mdw->setDisabled( !config->readEntry("Show", true) );

      } // inherits MixDeviceWidget
   } // for all MDW's
}

void KMixToolBox::loadKeys(ViewBase *view, KConfig *config)
// !!! this must be moved out of the views into the kmixd
{
   kDebug(67100) << "KMixToolBox::loadKeys()" << endl;
   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;
         KGlobalAccel *keys = KGlobalAccel::self();
         if ( keys )
         {
            QString devgrpkeys;
            devgrpkeys.sprintf( "Keys.%s.%s", view->getMixer()->id().ascii(), mdw->mixDevice()->id().ascii() );
            kDebug(67100) << "KMixToolBox::loadKeys() load Keys " << devgrpkeys << endl;

            // please see KMixToolBox::saveKeys() for some rambling about saving/loading Keys
            keys->setConfigGroup(devgrpkeys);
            keys->readSettings(config);
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
   grp += view->name();
   config->setGroup( grp );
   kDebug(67100) << "KMixToolBox::saveView() grp=" << grp.ascii() << endl;

   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;

         kDebug(67100) << "  grp=" << grp.ascii() << endl;
         kDebug(67100) << "  mixer=" << view->getMixer()->id().ascii() << endl;
         kDebug(67100) << "  mdwPK=" << mdw->mixDevice()->id().ascii() << endl;

         QString devgrp;
         devgrp.sprintf( "%s.%s.%s", grp.ascii(), view->getMixer()->id().ascii(), mdw->mixDevice()->id().ascii() );
         config->setGroup( devgrp );

         if ( mdw->inherits("MDWSlider") )
         {
            // only sliders have the ability to split apart in mutliple channels
            config->writeEntry( "Split", ! mdw->isStereoLinked() );
         }
         config->writeEntry( "Show" , ! mdw->isDisabled() );
      } // inherits MixDeviceWidget
   } // for all MDW's
}


// Save key bindings
void KMixToolBox::saveKeys(ViewBase *view, KConfig *config)
// !!! this must be moved out of the views into the kmixd
{
   /*
       Implementation hint: Conceptually keys SHOULD be bound to the actual hardware, and not
       to one GUI representation. Both work, but it COULD confuse users, if we have multiple
       GUI representations (e.g. "Dock Icon" and "Main Window").
       If you think about this aspect more deeply, you will find out that this is the case already
       today with "kmixapplet" and "kmix main application". It would really nice to rework this.
    */
   kDebug(67100) << "KMixToolBox::saveKeys()" << endl;
   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;
         KGlobalAccel *keys = KGlobalAccel::self();
         if ( keys )
         {
            QString devgrpkeys;
            devgrpkeys.sprintf( "Keys.%s.%s", view->getMixer()->id().ascii(), mdw->mixDevice()->id().ascii() );
            kDebug(67100) << "KMixToolBox::saveKeys() : " << devgrpkeys << endl;

            keys->setConfigGroup(devgrpkeys);
            keys->writeSettings(config);
         } // MDW has keys
      } // is a MixDeviceWidget
   } // for all widgets
}
