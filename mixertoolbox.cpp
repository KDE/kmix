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


#include "qwidget.h"
#include "qstring.h"

//#include <kdebug.h>
#include <klocale.h>

#include "mixdevice.h"
#include "mixer.h"

#include "mixertoolbox.h"

/***********************************************************************************
 Attention:
 This MixerToolBox is linked to the KMix Main Program, the KMix Applet and kmixctrl.
 As we do not want to link in more than neccesary to kmixctrl, you are asked
 not to put any GUI classes in here.
 In the case where it is unavoidable, please put them in KMixToolBox.
 ***********************************************************************************/
void MixerToolBox::initMixer(QPtrList<Mixer> &mixers, bool multiDriverMode, QString& ref_hwInfoString)
{
    // Find all mixers and initalize them
    QMap<QString,int> mixerNums;
    int drvNum = Mixer::getDriverNum();

    int driverWithMixer = -1;
    bool multipleDriversActive = false;

    QString driverInfo = "";
    QString driverInfoUsed = "";

    for( int drv=0; drv<drvNum ; drv++ )
    {
        QString driverName = Mixer::driverName(drv);
        if ( drv!= 0 )
            driverInfo += " + ";
        driverInfo += driverName;
    }

	/* Run a loop over all drivers. The loop will terminate after the first driver which
	   has mixers. And here is the reason:
	   - If you run ALSA with ALSA-OSS-Emulation enabled, mixers will show up twice: once
	     as native ALSA mixer, once as OSS mixer (emulated by ALSA). This is bad and WILL
	     confuse users. So it is a design decision that we can compile in multiple drivers
	     but we can run only one driver.
	   - For special usage scenarios, people will still want to run both drivers at the
	     same time. We allow them to hack their Config-File, where they can enable a
	     multi-driver mode.
	   - Another remark: For KMix3.0 or so, we should allow multiple-driver, for allowing
	     addition of special-use drivers, e.g. an ARTS-mixer-driver, or a CD-Rom volume driver.
	 */

    bool autodetectionFinished = false;
    for( int drv=0; drv<drvNum; drv++ )
        {
	    if ( autodetectionFinished ) {
		// sane exit from loop
		break;
	    }
	    bool drvInfoAppended = false;
	    // The "64" below is just a "silly" number:
	    // The loop will break as soon as an error is detected (e.g. on 3rd loop when 2 soundcards are installed)
	    for( int dev=0; dev<64; dev++ )
	    {
		Mixer *mixer = Mixer::getMixer( drv, dev );
                int mixerError = mixer->getErrno();
                if ( mixerError == 0 ) {
		    mixerError = mixer->grab();
                }
		if ( mixerError!=0 )
		{
		    if ( mixers.count() > 0 ) {
			// why not always ?!? !!
			delete mixer;
			mixer = 0;
		    }

		    /* If we get here, we *assume* that we probed the last dev of the current soundcard driver.
		     * We cannot be sure 100%, probably it would help to check the "mixerError" variable. But I
		     * currently don't see an error code that needs to be handled explicitely (Update:
		     *  We now have ERR_NODEV, which means, that a soundcard is present but has no Mixer.
		     *  It sounds pretty wise to probe the following soundcards of this driver. See Bug #87778).
		     *
		     * Lets decide if we the autoprobing shall continue:
		     */
		    if ( mixers.count() == 0 ) {
			// Simple case: We have no mixers. Lets go on with next driver
			// The only exception is Mixer::ERR_NODEV
			if (mixerError!=Mixer::ERR_NODEV)
			    continue;
			else
			    break;
		    }
		    else if ( multiDriverMode ) {
			// Special case: Multi-driver mode will probe more soundcards
			break;
		    }
		    else {
			// We have mixers, but no Multi-driver mode: Fine, we're done
			autodetectionFinished = true;
			break;
		    }
		}

		if ( mixer != 0 ) {
		    mixers.append( mixer );
		}

		// append driverName (used drivers)
		if ( !drvInfoAppended )
                {
		    drvInfoAppended = true;
		    QString driverName = Mixer::driverName(drv);
		    if ( drv!= 0 && mixers.count() > 0) {
			driverInfoUsed += " + ";
		    }
		    driverInfoUsed += driverName;
		}

		// Check whether there are mixers in different drivers, so that the user can be warned
		if (!multipleDriversActive)
		{
		    if ( driverWithMixer == -1 )
		    {
			// Aha, this is the very first detected device
			driverWithMixer = drv;
		    }
		    else
		    {
			if ( driverWithMixer != drv )
			{
			    // Got him: There are mixers in different drivers
			    multipleDriversActive = true;
			}
		    }
		}

		// count mixer nums for every mixer name to identify mixers with equal names
		mixerNums[mixer->mixerName()]++;
		mixer->setMixerNum( mixerNums[mixer->mixerName()] );
	    } // loop over sound card devices of current driver
	} // loop over soundcard drivers

	ref_hwInfoString = i18n("Sound drivers supported:");
	ref_hwInfoString += " " + driverInfo +
		"\n" + i18n("Sound drivers used:") + " " + driverInfoUsed;

	if ( multipleDriversActive )
	{
	    // this will only be possible by hacking the config-file, as it will not be officially supported
	    ref_hwInfoString += "\nExperimental multiple-Driver mode activated";
	}

	kdDebug(67100) << ref_hwInfoString << endl;
}

