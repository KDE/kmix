//-*-C++-*-

#ifndef MIXER_OSS4_H
#define MIXER_OSS4_H

#include <qstring.h>

#include "mixer_backend.h"
#include <sys/soundcard.h>

class Mixer_OSS4 : public Mixer_Backend
{
public:
  Mixer_OSS4(int device = -1);
  virtual ~Mixer_OSS4();

  virtual QString errorText(int mixer_error);
  virtual int readVolumeFromHW( int ctrlnum, Volume &vol );
  virtual int writeVolumeToHW( int ctrlnum, Volume &vol );
  virtual void setEnumIdHW(int ctrlnum, unsigned int idx);
  virtual unsigned int enumIdHW(int ctrlnum);
  virtual bool setRecsrcHW( int ctrlnum, bool on);
  virtual bool isRecsrcHW( int ctrlnum );

protected:

  MixDevice::ChannelType classifyAndRename(QString &name, int flags);

  int wrapIoctl(int ioctlRet);

  void reinitialize() { open(); close(); };
  virtual int open();
  virtual int close();
  virtual bool needsPolling() { return true; };

  int     m_ossVersion;
  int     m_fd;
  int     m_numExtensions;
  int     m_numMixers;
  QString m_deviceName;
};
#endif
