/**
 * This is an empty placeholder for the MM Sprint 2011 in Randa.
 * Quite likely it will not be activated for KDE4.7, but for the following release.
 */


#include "mixer_mpris2.h"

//#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QDBusReply>
 

#include <QString>

// Set the QDBUS_DEBUG env variable for debugging Qt DBUS calls.

QString Mixer_MPRIS2::MPRIS_IFC2; // = "org.mpris.MediaPlayer2";

Mixer_Backend* MPRIS2_getMixer(Mixer *mixer, int device )
{

   Mixer_Backend *l_mixer;

   l_mixer = new Mixer_MPRIS2(mixer,  device );
   return l_mixer;
}

Mixer_MPRIS2::Mixer_MPRIS2(Mixer *mixer, int device) : Mixer_Backend(mixer,  device )
{
//	run();
}


  int Mixer_MPRIS2::open()
  {
    if ( m_devnum !=  0 )
      return Mixer::ERR_OPEN;
    
    run();
    return 0;
  }

  int Mixer_MPRIS2::close()
  {
    return 0;    
  }

  int Mixer_MPRIS2::readVolumeFromHW( const QString& id, MixDevice * )
  {
    return 0;    
  }

  int Mixer_MPRIS2::writeVolumeToHW( const QString& id, MixDevice * )
  {
    return 0;    
  }
  
  void Mixer_MPRIS2::setEnumIdHW(const QString& id, unsigned int)
  {
  }
  
  unsigned int Mixer_MPRIS2::enumIdHW(const QString& id)
  {
    return 0;    
  }
  
  void Mixer_MPRIS2::setRecsrcHW( const QString& id, bool on)
  {
  }
  
  bool Mixer_MPRIS2::moveStream( const QString& id, const QString& destId )
  {
    return false;
  }


/**
 * @brief Test method
 *
 * @return int
 **/
int Mixer_MPRIS2::run()
{
  kDebug(67100) << "--- A ---------------------------------";
       if (!QDBusConnection::sessionBus().isConnected()) {
         kDebug() <<  "Cannot connect to the D-Bus session bus.\n"
                 << "To start it, run:\n"
                  <<"\teval `dbus-launch --auto-syntax`\n";
     }
     
  kDebug(67100) << "--- B ---------------------------------";
     QDBusConnection dbusConn = QDBusConnection::sessionBus();
     QDBusReply<QStringList> repl = dbusConn.interface()->registeredServiceNames();
     
  kDebug(67100) << "--- C ---------------------------------";
     if ( repl.isValid() )
     {
         kDebug(67100) << "--- D ---------------------------------";

	QStringList result = repl.value();
	QString s;
	foreach (  s , result )
	{
	  if ( s.startsWith("org.mpris.MediaPlayer2") )
	  {
	    getMprisControl(dbusConn, s);
	    
	  }
	}
     }

  
  return 0;
}



void Mixer_MPRIS2::getMprisControl(QDBusConnection& conn, QString busDestination)
{
qDebug() << "Get control of " << busDestination;

//  QDBusMessage query1 = QDBusMessage::createMethodCall(QString(busDestination), QString("/org/mpris/MediaPlayer2"), MPRIS_IFC2, QString("Raise"));
//  conn.call(query1, QDBus::NoBlock);

  QDBusMessage query = QDBusMessage::createMethodCall(QString(busDestination), QString("/org/mpris/MediaPlayer2"), "org.freedesktop.DBus.Properties", QString("Get"));
  QList<QVariant> arg;
  arg.append(QString("org.mpris.MediaPlayer2"));
  arg.append(QString("Identity"));
  query.setArguments(arg);
  
  //msg = conn.call(query, QDBus::Block, 5);
  QDBusMessage msg = conn.call(query, QDBus::Block, 1500);
  if ( msg.type() == QDBusMessage::ReplyMessage )
  {
    QList<QVariant> repl = msg.arguments();
    if ( ! repl.isEmpty() )
    {
      QVariant qv = repl.at(0);
	// We have to do some very ugly casting from QVariant to QDBusVariant to QVariant. This API totally sucks.
	QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(qv);
	QVariant result2 = dbusVariant.variant();
	QString readableName = result2.toString();
	
	qDebug() << "REPLY " << result2.type() << ": " << readableName;
	
	Volume::ChannelMask chn = (Volume::ChannelMask)(Volume::MLEFT | Volume::MRIGHT);
	MixDevice* md = new MixDevice(_mixer, readableName, readableName, MixDevice::VOLUME);
	Volume* vol = new Volume( chn, 100, 0, true, false);
	md->addPlaybackVolume(*vol);
	m_mixDevices.append( md );
    }
  }
  else
  {
    qWarning() << "Error (" << msg.type() << "): " << msg.errorName() << " " << msg.errorMessage();
  }
}


Mixer_MPRIS2::~Mixer_MPRIS2()
{
  
  
  
}


QString Mixer_MPRIS2::getDriverName()
{
	return "MPRIS2";
}

QString MPRIS2_getDriverName()
{
	return "MPRIS2";
}

//#include "Mixer_MPRIS2.moc"
