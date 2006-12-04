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

/**
 * Scan for Mixers in the System. This is the method that implicitely fills the
 * list of Mixer's, which is accesible via the static Mixer::mixer() method.
 * @par mixers The list where to add the found Mixers. This parameter is superfluous
 *             nowadays, as it is now really trivial to get it - just call the static
 *             Mixer::mixer() method.
 * @par multiDriverMode Whether the Mixer scan should try more all backendends.
 *          'true' means to scan all backends. 'false' means: After scanning the
 *          current backend the next backend is only scanned if no Mixers were found yet.
 */
void MixerToolBox::initMixer(QPtrList<Mixer> &mixers, bool multiDriverMode, QString& ref_hwInfoString)
{
   //kdDebug(67100) << "IN MixerToolBox::initMixer()"<<endl;

    // Find all mixers and initalize them
    QMap<QString,int> mixerNums;
    int drvNum = Mixer::numDrivers();

    int driverWithMixer = -1;
    bool multipleDriversActive = false;

    QString driverInfo = "";
    QString driverInfoUsed = "";

    for( int drv1=0; drv1<drvNum; drv1++ )
    {
      QString driverName = Mixer::driverName(drv1);
      if ( driverInfo.length() > 0 )
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
	     addition of special-use drivers, e.g. an Jack-Mixer-Backend, or a CD-Rom volume Backend.
	 */

    bool autodetectionFinished = false;
    for( int drv=0; drv<drvNum; drv++ )
    {
	  QString driverName = Mixer::driverName(drv);

	  if ( autodetectionFinished ) {
		// sane exit from loop
		break;
	    }

	    bool drvInfoAppended = false;
	    // The "19" below is just a "silly" number:
	    // (Old: The loop will break as soon as an error is detected - e.g. on 3rd loop when 2 soundcards are installed)
	    // New: We don't try be that clever anymore. We now blindly scan 20 cards, as the clever
	    // approach doesn't work for the one or other user.
	    int devNumMax = 19;
	    for( int dev=0; dev<=devNumMax; dev++ )
	    {
		Mixer *mixer = new Mixer( drv, dev );
		if ( mixer->isValid() ) {
			mixer->open();
			mixers.append( mixer );
			// Count mixer nums for every mixer name to identify mixers with equal names.
			// This is for creating persistent (reusable) primary keys, which can safely
			// be referenced (especially for config file access, so it is meant to be persistent!).
			mixerNums[mixer->mixerName()]++;
			// Create a useful PK
			/* As we use "::" and ":" as separators, the parts %1,%2 and %3 may not
			 * contain it.
			 * %1, the driver name is from the KMix backends, it does not contain colons.
			 * %2, the mixer name, is typically coming from an OS driver. It could contain colons.
			 * %3, the mixer number, is a number: it does not contain colons.
			 */
			QString mixerName = mixer->mixerName();
			mixerName.replace(":","_");
			QString primaryKeyOfMixer = QString("%1::%2:%3")
			    .arg(driverName)
			    .arg(mixerName)
			    .arg(mixerNums[mixer->mixerName()]);
			// The following 3 replaces are for not messing up the config file
			primaryKeyOfMixer.replace("]","_");
			primaryKeyOfMixer.replace("[","_"); // not strictly neccesary, but lets play safe
			primaryKeyOfMixer.replace(" ","_");
			primaryKeyOfMixer.replace("=","_");
			
			mixer->setID(primaryKeyOfMixer);

		} // valid
		else
		{
			delete mixer;
			mixer = 0;
		} // invalid

		/* Lets decide if we the autoprobing shall continue: */
		if ( multiDriverMode ) {
			// trivial case: In multiDriverMode, we scan ALL 20 devs of ALL drivers
			// so we have to do "nothing" in this case
		} // multiDriver
		else {
			// In No-multiDriver-mode we only need to check after we reached devNumMax
			if ( dev == devNumMax ) {
			   if ( Mixer::mixers().count() != 0 ) {
				// highest device number of driver and a Mixer => finished
				autodetectionFinished = true;	
			   }
			}
		} // !multiDriver

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
		} //  !multipleDriversActive
		
	    } // loop over sound card devices of current driver
	} // loop over soundcard drivers

        if ( Mixer::masterCard() == 0 ) {
           // We have no master card yet. This actually only happens when there was
           // not one defined in the kmixrc.
           // So lets just set the first card as master card.
           if ( Mixer::mixers().count() > 0 ) { 
              Mixer::setMasterCard( Mixer::mixers().first()->id());
           } 
        }

	ref_hwInfoString = i18n("Sound drivers supported:");
	ref_hwInfoString.append(" ").append( driverInfo ).append(	"\n").append(i18n("Sound drivers used:")) .append(" ").append(driverInfoUsed);

	if ( multipleDriversActive )
	{
	    // this will only be possible by hacking the config-file, as it will not be officially supported
	    ref_hwInfoString += "\nExperimental multiple-Driver mode activated";
	}

	kdDebug(67100) << ref_hwInfoString << endl << "Total number of detected Mixers: " << Mixer::mixers().count() << endl;
   //kdDebug(67100) << "OUT MixerToolBox::initMixer()"<<endl;

}


/*
 * Clean up and free all ressources of all found Mixers, which were found in the initMixer() call
 */
void MixerToolBox::deinitMixer()
{
   //kdDebug(67100) << "IN MixerToolBox::deinitMixer()"<<endl;
   Mixer *mixer;
   while ( (mixer=Mixer::mixers().first()) != 0)
   {
      //kdDebug(67100) << "MixerToolBox::deinitMixer() Remove Mixer" << endl;
      mixer->close();
      Mixer::mixers().remove(mixer);
      delete mixer;
   }
   // kdDebug(67100) << "OUT MixerToolBox::deinitMixer()"<<endl;
}
