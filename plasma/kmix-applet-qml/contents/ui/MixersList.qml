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
import org.kde.plasma.core 0.1 as PlasmaCore

ListView {
    id: _mixersList

    property alias model: _kmixModel

    clip: true
    spacing: 1

    // NOTE: I noticed, when lots of mixers are used, the popup applet icon and tooltip
    // weren't updated because the corresponding visual item wasn't rendered, therefore
    // the reliability of the tray will go nuts without this dynamic cacheBuffer
    // synchronization
    cacheBuffer: _kmixModel.count + 1
    delegate: orientation == Qt.Vertical ? _horizontalDelegate : _verticalDelegate
    interactive: orientation == Qt.Vertical ? height < contentHeight : width < contentWidth

    model: ListModel {
        id: _kmixModel
    }

    Component {
        id: _horizontalDelegate
        HorizontalMixerListDelegate {
            id: horDelegate

            PlasmaCore.FrameSvgItem {
                id: horizontalRect

                anchors {
                    top: horDelegate.top
                    left: horDelegate.left
                    right: horDelegate.right
                }
                width: _mixersList.width
                height: horDelegate.height

                imagePath: "widgets/listitem"
                prefix: "section"
                z: -1
            }
        }
    }

    Component {
        id: _verticalDelegate


        VerticalMixerListDelegate {
            id: verDelegate
 
            PlasmaCore.FrameSvgItem {
                id: verticalRect

                anchors.fill: verDelegate

                imagePath: "widgets/listitem"
                prefix: "section"
                z: -1
            }
        }
    }
}
