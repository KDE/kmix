#ifndef Mixer_MPRIS2_H
#define Mixer_MPRIS2_H

#include <QtGui/QMainWindow>
#include <QtDBus/QtDBus>

Mixer_MPRIS2 : public Mixer_Backend
{
	explicit Mixer_MPRIS2(Mixer *mixer, int device = -1 );
    ~Mixer_MPRIS2();
    void getMprisControl(QDBusConnection& conn, QString arg1);
}

#endif

