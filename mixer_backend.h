//-*-C++-*-
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

#ifndef MIXER_BACKEND_H
#define MIXER_BACKEND_H

#include "mixer.h"

class Mixer_Backend
{
// The Mixer Backend's may only be accessed from the Mixer class.
friend class Mixer;

protected:
  Mixer_Backend(int devnum);
  virtual ~Mixer_Backend();

  /// Derived classes MUST implement this to open the mixer. Returns a KMix error code (O=OK).
  virtual int open() = 0;
  virtual int close() = 0;

  /** Returns, whether this Mixer object contains a valid Mixer. You should return "false", when
   * the Mixer with the devnum given in the constructor is not supported by the Backend. The two
   * typical cases are:
   * (1) No such hardware installed
   * (2) The hardware exists, but has no mixer support (e.g. external soundcard with only mechanical volume knobs)
   * The default implementation calls open(), checks the return code and whether the number of
   * supported channels is > 0. Then it calls close().
   * You should reimplement this method in your backend, when there is a less time-consuming method than
   * calling open() and close() for checking the existance of a Mixer.
   */
  virtual bool isValid();

  /** @return true, if the Mixer is open (and thus can be operated) */
  bool isOpen();

  virtual bool prepareUpdateFromHW();

  /// Volume Read
  virtual int readVolumeFromHW( int devnum, Volume &vol ) = 0;
  /// Volume Write
  virtual int writeVolumeToHW( int devnum, Volume &volume ) = 0;

  /// Enums
  virtual void setEnumIdHW(int mixerIdx, unsigned int);
  virtual unsigned int enumIdHW(int mixerIdx);

  /// Recording Switches
  virtual bool setRecsrcHW( int devnum, bool on) = 0;
  virtual bool isRecsrcHW( int devnum ) = 0;

  /// Overwrite in the backend if the backend can see changes without polling
  virtual bool needsPolling() { return true; }

  /** overwrite this if you need to connect to slots in the mixer (e.g. readSetFromHW)
      this called in the very beginning and only if !needsPolling
  */
  virtual void prepareSignalling( Mixer * ) {}

  MixDevice* recommendedMaster();

  /** Return a translated error text for the given error number.
   * Subclasses can override this method to produce platform
   * specific error descriptions.
   */
  virtual QString errorText(int mixer_error);
  /// Prints out a translated error text for the given error number on stderr
  void errormsg(int mixer_error);


  int m_devnum;
  /// User friendly name of the Mixer (e.g. "IRIX Audio Mixer"). If your mixer API
  /// gives you a usable name, use that name.
  QString m_mixerName;
  // All mix devices of this phyisical device.
  MixSet m_mixDevices;

  /******************************************************************************************
   * Please don't access the next vars from the Mixer class (even though Mixer is a friend).
   * There are proper accesor methods for them.
   ******************************************************************************************/
  bool m_isOpen;
  // The MixDevice that would qualify best as MasterDevice (according to the taste of the Backend developer)
  MixDevice* m_recommendedMaster;
};

#endif
