#include "ControlGroupTab.h"
#include "ControlSlider.h"
#include <QtGui/QHBoxLayout>

#include "controlgroup_interface.h"
#include "control_interface.h"

const QString KMIX_DBUS_SERVICE = "org.kde.kmixd";
const QString KMIX_DBUS_PATH = "/KMixD";

ControlGroupTab::ControlGroupTab(org::kde::KMix::ControlGroup *group, QWidget *parent)
    : QWidget(parent)
    , m_group(group)
{
    QHBoxLayout *m_layout = new QHBoxLayout(this);
    setLayout(m_layout);
    foreach(const QString &controlName, group->controls()) {
        org::kde::KMix::Control *control = new org::kde::KMix::Control(KMIX_DBUS_SERVICE, controlName, QDBusConnection::sessionBus());
        ControlSlider *slider = new ControlSlider(control, this);
        m_layout->addWidget(slider);
    }
}

ControlGroupTab::~ControlGroupTab()
{
}
