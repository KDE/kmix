#include <kapp.h>
#include <dcopclient.h>

#include <stdlib.h>

int main(int argc, char **argv)
{
  if (argc != 3) {
    qDebug("Usage: kmix profile <set-number>      Loads a mixer profile.");
    return 1;
  }

  KApplication app(argc, argv, "kmixclient");

  QByteArray data;
  DCOPClient *client; client = app.dcopClient();

  client->attach(); // attach to the server, now we can use DCOP service
  client->registerAs( app.name() ); // register at the server, now others can call us.

  if ( ! client->isApplicationRegistered( app.name() ) ) {
    // Fail
    qDebug("kmixclient: Failed in registering to DCOP.");
  }
  else {
    // OK, send data
    int l_i_setNum = atoi(argv[2]);
    QDataStream ds(data, IO_WriteOnly);
    ds << l_i_setNum;

    if (!client->send("kmix", "KMix", "activateSet(  int  )", data ) )
      qDebug("Error sending to KMix");
  }

  client->detach();
  return 0;
}
