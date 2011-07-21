#ifndef Mixer_MPRIS2_H
#define Mixer_MPRIS2_H

#include <QString>
#include <QMainWindow>
#include <QtDBus>
#include <QDBusInterface>
#include <QMap>

#include "mixer_backend.h"

class MPrisAppdata
{
public:
  QString id;
  QDBusInterface* propertyIfc;
};

class Mixer_MPRIS2 : public Mixer_Backend
{
public:
  explicit Mixer_MPRIS2(Mixer *mixer, int device = -1 );
    ~Mixer_MPRIS2();
    void getMprisControl(QDBusConnection& conn, QString arg1);
    QString getDriverName();

  virtual int open();
  virtual int close();
  virtual int readVolumeFromHW( const QString& id, MixDevice * );
  virtual int writeVolumeToHW( const QString& id, MixDevice * );
  virtual void setEnumIdHW(const QString& id, unsigned int);
  virtual unsigned int enumIdHW(const QString& id);
  virtual void setRecsrcHW( const QString& id, bool on);
  virtual bool moveStream( const QString& id, const QString& destId );
  virtual bool needsPolling() { return false; }


private:
  int run();
//  static char MPRIS_IFC2[40];
  static QString MPRIS_IFC2;
  
  static QString getBusDestination(const QString& id);
  
  QMap<QString,MPrisAppdata> apps;
};





#endif

