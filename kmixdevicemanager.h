#ifndef kmixdevicemanager_h
#define kmixdevicemanager_h

#include <QObject>

class kdm : public QObject
{
  Q_OBJECT

public:
   kdm();

public slots:
   void plugged(const QString&);
   void tick();
};

#endif

