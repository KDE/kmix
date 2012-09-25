#include "ControlMonitor.h"
#include "control_interface.h"
#include "controlmonitoradaptor.h"
#include <QtGui/QProgressBar>

QAtomicInt ControlMonitor::s_id = 0;

ControlMonitor::ControlMonitor(QProgressBar *widget, org::kde::KMix::Control *control, QObject *parent)
    : QObject(parent)
    , m_widget(widget)
{
    new ControlMonitorAdaptor(this);
    QString path = QString("/org/kde/kmix/monitors/%1").arg(s_id.ref());
    QDBusConnection::sessionBus().registerObject(path, this);
    control->subscribeMonitor(QDBusConnection::sessionBus().baseService(), path);
}

ControlMonitor::~ControlMonitor()
{
    emit removed();
}

void ControlMonitor::levelUpdate(int level)
{
    qDebug() << "Level update to" << level;
    m_widget->setValue(level);
}

#include "ControlMonitor.moc"
