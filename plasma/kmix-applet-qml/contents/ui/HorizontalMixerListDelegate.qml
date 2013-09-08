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
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.qtextracomponents 0.1 as QtExtras

Column {

    anchors {
        left: parent.left
        right: parent.right
        leftMargin: _controlInfo.spacing
    }

    width: parent.width

    Row {
        id: _controlInfo

        property alias icon: _controlIcon.icon
        property alias text: _controlText.text

        spacing: 5

        QtExtras.QIconItem {
            id: _controlIcon

            anchors {
                verticalCenter: parent.verticalCenter
            }

            width: theme.iconSizes.toolbar
            height: width
        }

        PlasmaComponents.Label {
            id: _controlText

            anchors {
                verticalCenter: parent.verticalCenter
            }

            elide: Text.ElideRight
        }
    }

    HorizontalControl {

        width: parent.width

        dataSource: _source
        isMasterControl: _isMasterControl
    }
}
