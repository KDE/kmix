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

#include "mdwslider.h"
#include "mdwswitch.h"
#include "mixdevicewidget.h"
#include "mixdevice.h"

#include "kmixtoolbox.h"

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
	if ( qmdw->inherits("MDWSlider") ) { // -<- play safe here
	    static_cast<MDWSlider*>(qmdw)->setTicks( on );
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
            /* !!! to be done
	    devgrp.sprintf( "%s.%s.Dev%s", viewPrefix.ascii(), grp.ascii(), mdw->mixDevice()->getPK().ascii() );
	    if ( ! config->hasGroup(devgrp) ) {
		// fall back to old config style
            */
		devgrp.sprintf( "%s.%s.Dev%i", viewPrefix.ascii(), grp.ascii(), n );
	    //}
	    config->setGroup( devgrp );

	    if ( qmdw->isA("MDWSlider") ) {
		// only sliders have the ability to split apart in mutliple channels
		MDWSlider *mdws = static_cast<MDWSlider*>(mdw);
		bool splitChannels = config->readBoolEntry("Split", false);
		mdws->setStereoLinked( !splitChannels );
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
	    devgrp.sprintf( "%s.%s.Dev%s", viewPrefix.ascii(), grp.ascii(), mdw->mixDevice()->getPK().ascii() );
	    //devgrp.sprintf( "%s.%s.Dev%i", viewPrefix.ascii(), grp.ascii(), n );
	    config->setGroup( devgrp );

	    if ( qmdw->isA("MDWSlider") ) {
		// only sliders have the ability to split apart in mutliple channels
		MDWSlider *mdws = static_cast<MDWSlider*>(mdw);
		config->writeEntry( "Split", ! mdws->isStereoLinked() );
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


