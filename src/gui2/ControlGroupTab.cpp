#include "ControlGroupTab.h"
#include "ControlSlider.h"
#include <QtGui/QVBoxLayout>

#include "controlgroup_interface.h"
#include "control_interface.h"

const QString KMIX_DBUS_SERVICE = "org.kde.kmixd";
const QString KMIX_DBUS_PATH = "/KMixD";

ControlGroupTab::ControlGroupTab(org::kde::KMix::ControlGroup *group, QWidget *parent)
    : QWidget(parent)
    , m_group(group)
{
    m_layout = new QVBoxLayout(this);
    setLayout(m_layout);
    connect(group, SIGNAL(controlAdded(QString)), this, SLOT(controlAdded(QString)));
    connect(group, SIGNAL(controlRemoved(QString)), this, SLOT(controlRemoved(QString)));
    foreach(const QString &controlName, group->controls()) {
        controlAdded(controlName);
    }
}

ControlGroupTab::~ControlGroupTab()
{
}

void ControlGroupTab::controlAdded(const QString &path)
{
    qDebug() << "Discovered control" << path;
    org::kde::KMix::Control *control = new org::kde::KMix::Control(KMIX_DBUS_SERVICE, path, QDBusConnection::sessionBus());
    ControlSlider *slider = new ControlSlider(control, this);
    m_layout->addWidget(slider);
    m_controls[path] = slider;
}

void ControlGroupTab::controlRemoved(const QString &path)
{
    qDebug() << "Lost control" << path;
    ControlSlider *slider = m_controls[path];
    slider->deleteLater();
    m_layout->removeWidget(slider);
}
