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

Column {
    id: _buttonContainer
    anchors {
        left: parent.left
        right: parent.right
        bottom: parent.bottom
    }

    spacing: 5
    PlasmaCore.SvgItem {
        id: separator
        height: lineSvg.elementSize( "horizontal-line" ).height
        svg: PlasmaCore.Svg {
            id: lineSvg
            imagePath: "widgets/line"
        }
        elementId: "horizontal-line"
        anchors {
            left: parent.left
            right: parent.right
        }
    }

    Row {
        spacing: 5
        anchors {
            horizontalCenter: parent.horizontalCenter
        }

        PlasmaComponents.ToolButton {
            id: _restoreKMix
            flat: false
            text: i18n( "KMix setup" )
            iconSource: "kmix"
            onClicked: action_kmixSetup()
        }

        PlasmaComponents.ToolButton {
            id: _phononSetup
            flat: false
            text: i18n( "Phonon setup" )
            iconSource: "preferences-desktop-sound"
            onClicked: action_phononSetup()
        }
    }

    PlasmaCore.DataSource {
        id: executable
        engine: "executable"
        onSourceAdded: removeSource( source )
    }

    function execute( app ) {
        executable.connectSource( app )
    }
}
