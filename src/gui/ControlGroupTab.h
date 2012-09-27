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

#ifndef CONTROLGROUPTAB_H
#define CONTROLGROUPTAB_H

#include <QtGui/QWidget>
#include <QtCore/QHash>

class QLayout;
class ControlSlider;
class QResizeEvent;
class QLabel;

class OrgKdeKMixControlGroupInterface;
namespace org {
    namespace kde {
        namespace KMix {
            typedef OrgKdeKMixControlGroupInterface ControlGroup;
        }
    }
}

class ControlGroupTab : public QWidget {
    Q_OBJECT
public:
    ControlGroupTab(org::kde::KMix::ControlGroup *group, QWidget *parent);
    ~ControlGroupTab();
protected:
    void resizeEvent(QResizeEvent *evt);
private slots:
    void controlAdded(const QString &path);
    void controlRemoved(const QString &path);
private:
    org::kde::KMix::ControlGroup *m_group;
    QLayout *m_layout;
    QHash<QString, ControlSlider*> m_controls;
    QLabel *m_emptyLabel;
};

#endif // CONTROLGROUPTAB_H
