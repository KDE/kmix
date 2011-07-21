#ifndef Mixer_MPRIS2_H
#define Mixer_MPRIS2_H

#include <QString>
#include <QMainWindow>
#include <QtDBus>
#include <QDBusInterface>
#include <QMap>

#include "mixer_backend.h"

class MPrisAppdata : public QObject
{
  Q_OBJECT 
public:
  MPrisAppdata();
  ~MPrisAppdata();
  QString id;
  QDBusInterface* propertyIfc;
  QDBusInterface* playerIfc;

public slots:
//  void volumeChangedIncoming(QString ifc, QList<QVariant> msg);
  void volumeChangedIncoming(QString,QVariantMap,QStringList);
  
signals:
  void volumeChanged(MPrisAppdata* mad, double);
};

class Mixer_MPRIS2 : public Mixer_Backend
{
  Q_OBJECT
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

  virtual int mediaPlay(QString id);
  virtual int mediaPrev(QString id);
  virtual int mediaNext(QString id);
  virtual int mediaControl(QString id, QString command);

public slots:
  void volumeChanged(MPrisAppdata* mad, double);

private:
  int run();
//  static char MPRIS_IFC2[40];
  static QString MPRIS_IFC2;
  
  static QString getBusDestination(const QString& id);
  
  QMap<QString,MPrisAppdata*> apps;
};

#endif

