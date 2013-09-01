// -*- coding: iso-8859-1 -*-
/*
 *   Author: Diego [Po]lentino Casella <polentino911@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 1.1
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.qtextracomponents 0.1

Column {
    id: _controlInfo
    property alias icon: _controlIcon.icon
    property variant text: undefined
    spacing: 5

    QIconItem {
        id: _controlIcon
        width: theme.iconSizes.toolbar
        height: width
        anchors {
            horizontalCenter: parent.horizontalCenter
        }
    }

    VerticalControl {
        dataSource: _source
        isMasterControl: _isMasterControl
    }
}