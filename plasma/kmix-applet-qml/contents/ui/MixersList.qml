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

ListView {
    id: _mixersList
    property alias model: _kmixModel
    orientation: QtVertical
    clip: true
    anchors {
        top: parent.top
        left: parent.left
        right: parent.right
        bottom: _buttonBar.top
    }

    model: ListModel {
        id: _kmixModel
    }

    delegate: orientation == QtVertical ? _horizontalDelegate : _verticalDelegate
    
    Component {
        id: _horizontalDelegate
        HorizontalMixerListDelegate {}
    }
    
    Component {
        id: _verticalDelegate
        VerticalMixerListDelegate {}
    }
}