#include "ControlGroupTab.h"
#include "ControlSlider.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QResizeEvent>
#include <QtGui/QLabel>

#include "controlgroup_interface.h"
#include "control_interface.h"

const QString KMIX_DBUS_SERVICE = "org.kde.kmixd";
const QString KMIX_DBUS_PATH = "/KMixD";

ControlGroupTab::ControlGroupTab(org::kde::KMix::ControlGroup *group, QWidget *parent)
    : QWidget(parent)
    , m_group(group)
{
    m_layout = new QVBoxLayout(this);
    m_emptyLabel = new QLabel(tr("No controls available."), this);
    m_emptyLabel->setGeometry(geometry());
    m_emptyLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_emptyLabel->setEnabled(false);
    m_emptyLabel->show();
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

void ControlGroupTab::resizeEvent(QResizeEvent *evt)
{
    m_emptyLabel->setGeometry(geometry());
}

void ControlGroupTab::controlAdded(const QString &path)
{
    qDebug() << "Discovered control" << path;
    org::kde::KMix::Control *control = new org::kde::KMix::Control(KMIX_DBUS_SERVICE, path, QDBusConnection::sessionBus());
    ControlSlider *slider = new ControlSlider(control, this);
    m_layout->addWidget(slider);
    m_controls[path] = slider;
    m_emptyLabel->hide();
}

void ControlGroupTab::controlRemoved(const QString &path)
{
    qDebug() << "Lost control" << path;
    ControlSlider *slider = m_controls.take(path);
    slider->deleteLater();
    m_layout->removeWidget(slider);
    if (m_controls.size() == 0) {
        m_emptyLabel->show();
    }
}
