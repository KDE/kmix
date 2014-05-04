/*
 *   Author: 2013 Diego [Po]lentino Casella <polentino911@gmail.com>
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
import org.kde.qtextracomponents 0.1 as QtExtras

Column {
    id: _controlInfo

    anchors {
        top: parent.top
        bottom: parent.bottom
    }

    property alias icon: _controlIcon.icon
    property variant text: undefined

    spacing: 5

    QtExtras.QIconItem {
        id: _controlIcon

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: _controlInfo.top
            topMargin: _controlInfo.spacing
        }

        width: theme.iconSizes.toolbar
        height: width
    }

    VerticalControl {
        id: _verticalControl

        anchors {
            top: _controlIcon.bottom
            bottomMargin: _controlInfo.spacing
            bottom: parent.bottom
        }

        dataSource: _source
        isMasterControl: _isMasterControl
    }
}
