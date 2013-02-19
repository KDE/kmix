/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) Trever Fischer <tdfischer@fedoraproject.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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
    m_layout->addStretch();
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
    QWidget::resizeEvent(evt);
}

void ControlGroupTab::controlAdded(const QString &path)
{
    qDebug() << "Discovered control" << path;
    org::kde::KMix::Control *control = new org::kde::KMix::Control(KMIX_DBUS_SERVICE, path, QDBusConnection::sessionBus());
    ControlSlider *slider = new ControlSlider(control, this);
    m_layout->insertWidget(m_layout->count()-1, slider);
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
