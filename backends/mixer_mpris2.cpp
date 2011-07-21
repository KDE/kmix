/**
 * This is an empty placeholder for the MM Sprint 2011 in Randa.
 * Quite likely it will not be activated for KDE4.7, but for the following release.
 */


#include "mixer_mpris2.h"

//#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QDBusReply>
 
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>


// Set the QDBUS_DEBUG env variable for debugging Qt DBUS calls.

QString Mixer_MPRIS2::MPRIS_IFC2 = "org.mpris.MediaPlayer2";

Mixer_Backend* MPRIS2_getMixer(Mixer *mixer, int device )
{

   Mixer_Backend *l_mixer;

   l_mixer = new MPRIS2(mixer,  device );
   return l_mixer;
}

Mixer_MPRIS2(Mixer *mixer, int device = -1 ) : Mixer_Backend(mixer,  device )
{
	run();
}

/**
 * @brief Test method
 *
 * @return int
 **/
int Mixer_MPRIS2::run()
{
       if (!QDBusConnection::sessionBus().isConnected()) {
         qDebug() <<  "Cannot connect to the D-Bus session bus.\n"
                 << "To start it, run:\n"
                  <<"\teval `dbus-launch --auto-syntax`\n";
     }
     
     /**
      * @brief ...
      **/
     QDBusConnection dbusConn = QDBusConnection::sessionBus();
     QDBusReply<QStringList> repl = dbusConn.interface()->registeredServiceNames();
     
     if ( repl.isValid() )
     {
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


#include "Mixer_MPRIS2.moc"
