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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include "qcolor.h"
#include "qwidget.h"
#include "qstring.h"

//#include <kdebug.h>
#include <kglobalaccel.h>
#include <klocale.h>

#include "mdwslider.h"
#include "mdwswitch.h"
#include "mixdevicewidget.h"
#include "mixdevice.h"
#include "mixer.h"

#include "kmixtoolbox.h"

/***********************************************************************************
 KMixToolbox contains several GUI relevant methods that are shared between the 
 KMix Main Program, and the KMix Applet.
 kmixctrl - as not non-GUI application - does NOT link to KMixToolBox.

 This means: Shared GUI stuff goes into the KMixToolBox class , non-GUI stuff goes
 into the MixerToolBox class.
 ***********************************************************************************/
void KMixToolBox::setIcons(QPtrList<QWidget> &mdws, bool on ) {
    for ( QWidget *qmdw=mdws.first(); qmdw!=0; qmdw=mdws.next() ) {
	if ( qmdw->inherits("MixDeviceWidget") ) { // -<- play safe here
	    static_cast<MixDeviceWidget*>(qmdw)->setIcons( on );
	}
    }
}

void KMixToolBox::setLabels(QPtrList<QWidget> &mdws, bool on ) {
    QWidget *qmdw;
    for ( qmdw=mdws.first(); qmdw != 0; qmdw=mdws.next() ) {
	if ( qmdw->inherits("MixDeviceWidget") ) { // -<- play safe here
	    static_cast<MixDeviceWidget*>(qmdw)->setLabeled( on );
	}
    }
}

void KMixToolBox::setTicks(QPtrList<QWidget> &mdws, bool on ) {
    QWidget *qmdw;
    for ( qmdw=mdws.first(); qmdw != 0; qmdw=mdws.next() ) {
	if ( qmdw->inherits("MixDeviceWidget") ) { // -<- in reality it is only in MDWSlider
	    static_cast<MixDeviceWidget*>(qmdw)->setTicks( on );
	}
    }
}

void KMixToolBox::setValueStyle(QPtrList<QWidget> &mdws, int vs ) {
    QWidget *qmdw;
    for ( qmdw=mdws.first(); qmdw != 0; qmdw=mdws.next() ) {
	if ( qmdw->inherits("MixDeviceWidget") ) { // -<- in reality it is only in MDWSlider
	  static_cast<MixDeviceWidget*>(qmdw)->setValueStyle( (MixDeviceWidget::ValueStyle) vs );
	}
    }
}

void KMixToolBox::loadConfig(QPtrList<QWidget> &mdws, KConfig *config, const QString &grp, const QString &viewPrefix) {
    int n = 0;
    config->setGroup( grp );
    int num = config->readNumEntry( viewPrefix + ".Devs", 0);

    for ( QWidget *qmdw=mdws.first(); qmdw!=0 && n<num; qmdw=mdws.next() ) {
	if ( qmdw->inherits("MixDeviceWidget") ) { // -<- play safe here
	    MixDeviceWidget* mdw = static_cast<MixDeviceWidget*>(qmdw);
	    QString devgrp;

	    /*
	     * Compatibility config loader! We use the old config group only, if the
	     * new one does not exist.
	     * The new group system has been introduced, because it accounts much
	     * better for soundcard driver updates (if numbering changes, or semantics
	     * of an ID changes like ALSA changing from "Disable Amplifier" to "External Amplifier").
	     */
            // !!! check
 	    devgrp.sprintf( "%s.%s.Dev%s", viewPrefix.ascii(), grp.ascii(), mdw->mixDevice()->getPK().ascii() );

            /** 
               Find an appropriate group name for capture GUI elements.
               We try  devgrp.append(".Capture")
               If it doesn't exist, we fall back to devgrp.
               This is the second compatibility measure, and was introduced for KDE3.5.2.
              */
            if ( mdw->mixDevice()->getVolume().isCapture() ) {
               /* A "capture" GUI element must save its own state. Otherwise playback and capture
                  properties would be written twice under the same name. This would mean, when
                 restoring, both would get the same value. This is bad, because hidden sliders will re-appear
                  after restart of KMix, and a lot of other nasty GUI-related problems.
                  So we add ".Capture" to the group name.
                  See bug 121451 "KMix panel applet shows broken duplicates of bass, treble sliders"

                  The name should have been set in the backend class, but we REALLY cannot do this for KDE3.5.x. !!
                  This issue will be fixed in KDE4 by the great config cleanup.
                */
               QString devgrpTmp(devgrp);
               devgrpTmp.append(".Capture");
               if ( config->hasGroup(devgrpTmp) ) {
                  // Group for capture device exists => take over the name
                  devgrp = devgrpTmp;
               }
               else {
                  // do nothing => keep old name (devgrp).
                  // Saving wil autmatically create the group 'devgrp.append(".Capture")'
                  kdDebug(67100) << "KMixToolBox::loadConfig() capture fallback activcated. Fallback group is " << devgrp << endl;
               }
            } // isCapture()
	    if ( ! config->hasGroup(devgrp) ) {
		// fall back to old-Style configuration (KMix2.1 and earlier)
		devgrp.sprintf( "%s.%s.Dev%i", viewPrefix.ascii(), grp.ascii(), n );
		// this configuration group will be deleted when config is saved
	    }
	    config->setGroup( devgrp );

	    if ( qmdw->inherits("MixDeviceWidget") ) { // -<- in reality it is only in MDWSlider
		// only sliders have the ability to split apart in mutliple channels
		bool splitChannels = config->readBoolEntry("Split", false);
		mdw->setStereoLinked( !splitChannels );
	    }
	    mdw->setDisabled( !config->readBoolEntry("Show", true) );

	    KGlobalAccel *keys=mdw->keys();
	    if ( keys )
	    {
		QString devgrpkeys;
		devgrpkeys.sprintf( "%s.%s.Dev%i.keys", viewPrefix.ascii(), grp.ascii(), n );
		//kdDebug(67100) << "KMixToolBox::loadConfig() load Keys " << devgrpkeys << endl;

		// please see KMixToolBox::saveConfig() for some rambling about saving/loading Keys
		keys->setConfigGroup(devgrpkeys);
		keys->readSettings(config);
		keys->updateConnections();
	    }

	    n++;
	} // if it is a MixDeviceWidget
    } // for all widgets
}


void KMixToolBox::saveConfig(QPtrList<QWidget> &mdws, KConfig *config, const QString &grp, const QString &viewPrefix) {
    config->setGroup( grp  );
    config->writeEntry( viewPrefix + ".Devs", mdws.count() );

    int n=0;
    for ( QWidget *qmdw=mdws.first(); qmdw!=0; qmdw=mdws.next() ) {
	if ( qmdw->inherits("MixDeviceWidget") ) { // -<- play safe here
	    MixDeviceWidget* mdw = static_cast<MixDeviceWidget*>(qmdw);

	    QString devgrp;
	    devgrp.sprintf( "%s.%s.Dev%i", viewPrefix.ascii(), grp.ascii(), n );
	    if ( ! config->hasGroup(devgrp) ) {
		// old-Style configuration (KMix2.1 and earlier => remove now unused group
		config->deleteGroup(devgrp);
            }
	    devgrp.sprintf( "%s.%s.Dev%s", viewPrefix.ascii(), grp.ascii(), mdw->mixDevice()->getPK().ascii() );
	    //devgrp.sprintf( "%s.%s.Dev%i", viewPrefix.ascii(), grp.ascii(), n );

            if ( mdw->mixDevice()->getVolume().isCapture() ) {
               /* see loadConfig() for the rationale of having an own name for capture devices. */
               devgrp.append(".Capture");
            } // isCapture()

	    config->setGroup( devgrp );

	    if ( qmdw->inherits("MixDeviceWidget") ) { // -<- in reality it is only in MDWSlider
		// only sliders have the ability to split apart in mutliple channels
		config->writeEntry( "Split", ! mdw->isStereoLinked() );
	    }
	    config->writeEntry( "Show" , ! mdw->isDisabled() );

	    // Save key bindings
	    /*
	       Implementation hint: Conceptually keys SHOULD be bound to the actual hardware, and not
	       to one GUI representation. Both work, but it COULD confuse users, if we have multiple
	       GUI representations (e.g. "Dock Icon" and "Main Window").
	       If you think about this aspect more deeply, you will find out that this is the case already
	       today with "kmixapplet" and "kmix main application". It would really nice to rework this.
	    */
	    KGlobalAccel *keys=mdw->keys();
	    if (keys) {
		QString devgrpkeys;
		devgrpkeys.sprintf( "%s.%s.Dev%i.keys", viewPrefix.ascii(), grp.ascii(), n );
		//kdDebug(67100) << "KMixToolBox::saveConfig() save Keys " << devgrpkeys << endl;
		keys->setConfigGroup(devgrpkeys);
		keys->writeSettings(config);
	    }
	    n++;
	} // if it is a MixDeviceWidget
    } // for all widgets
}

