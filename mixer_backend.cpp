/*
 * KMix -- KDE's full featured mini mixer
 *
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

#include <klocale.h>

#include "mixer_backend.h"
// for the "ERR_" declartions, #include mixer.h
#include "mixer.h"

Mixer_Backend::Mixer_Backend(int device) :
   m_devnum (device) , m_isOpen(false), m_recommendedMaster(0)
{
  m_mixDevices.setAutoDelete( true );
}

Mixer_Backend::~Mixer_Backend()
{
}


bool Mixer_Backend::isValid() {
	bool valid = false;
	int ret = open();
	if ( ret == 0  && m_mixDevices.count() > 0) {
	  valid = true;
	}
	close();
	return valid;
}

bool Mixer_Backend::isOpen() {
  return m_isOpen;
}

/**
 * Queries the backend driver whether there are new changes in any of the controls.
 * If you cannot find out for a backend, return "true" - this is also the default implementation.
 * @return true, if there are changes. Otherwise false is returned.
 */
bool Mixer_Backend::prepareUpdateFromHW() {
  return true;
}

/**
 * Return the MixDevice, that would qualify best as MasterDevice. The default is to return the
 * first device in the device list. Backends can override this (i.e. the ALSA Backend does so).
 * The users preference is NOT returned by this method - see the Mixer class for that.
 */
MixDevice* Mixer_Backend::recommendedMaster() {
   MixDevice* recommendedMixDevice = 0;
   if ( m_recommendedMaster != 0 ) {
      recommendedMixDevice = m_recommendedMaster;
   } // recommendation from Backend
   else {
      if ( m_mixDevices.count() > 0 ) {
         recommendedMixDevice = m_mixDevices.at(0);
      } //first device (if exists)
   }
   return recommendedMixDevice;
}

/**
 * Sets the ID of the currently selected Enum entry.
 * This is a dummy implementation - if the Mixer backend
 * wants to support it, it must implement the driver specific 
 * code in its subclass (see Mixer_ALSA.cpp for an example).
 */
void Mixer_Backend::setEnumIdHW(int, unsigned int) {
  return;
}

/**
 * Return the ID of the currently selected Enum entry.
 * This is a dummy implementation - if the Mixer backend
 * wants to support it, it must implement the driver specific
 * code in its subclass (see Mixer_ALSA.cpp for an example).
 */
unsigned int Mixer_Backend::enumIdHW(int) {
  return 0;
}

void Mixer_Backend::errormsg(int mixer_error)
{
  QString l_s_errText;
  l_s_errText = errorText(mixer_error);
  kdError() << l_s_errText << "\n";
}

QString Mixer_Backend::errorText(int mixer_error)
{
  QString l_s_errmsg;
  switch (mixer_error)
  {
    case Mixer::ERR_PERM:
      l_s_errmsg = i18n("kmix:You do not have permission to access the mixer device.\n" \
	  "Please check your operating systems manual to allow the access.");
      break;
    case Mixer::ERR_WRITE:
      l_s_errmsg = i18n("kmix: Could not write to mixer.");
      break;
    case Mixer::ERR_READ:
      l_s_errmsg = i18n("kmix: Could not read from mixer.");
      break;
    case Mixer::ERR_NODEV:
      l_s_errmsg = i18n("kmix: Your mixer does not control any devices.");
      break;
    case  Mixer::ERR_NOTSUPP:
      l_s_errmsg = i18n("kmix: Mixer does not support your platform. See mixer.cpp for porting hints (PORTING).");
      break;
    case  Mixer::ERR_NOMEM:
      l_s_errmsg = i18n("kmix: Not enough memory.");
      break;
    case Mixer::ERR_OPEN:
    case Mixer::ERR_MIXEROPEN:
      // ERR_MIXEROPEN means: Soundcard could be opened, but has no mixer. ERR_MIXEROPEN is normally never
      //      passed to the errorText() method, because KMix handles that case explicitely
      l_s_errmsg = i18n("kmix: Mixer cannot be found.\n" \
	  "Please check that the soundcard is installed and that\n" \
	  "the soundcard driver is loaded.\n");
      break;
    case Mixer::ERR_INCOMPATIBLESET:
      l_s_errmsg = i18n("kmix: Initial set is incompatible.\n" \
	  "Using a default set.\n");
      break;
    default:
      l_s_errmsg = i18n("kmix: Unknown error. Please report how you produced this error.");
      break;
  }
  return l_s_errmsg;
}

