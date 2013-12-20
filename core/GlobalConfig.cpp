/*
 KMix -- KDE's full featured mini mixer
 Copyright (C) 2012  Christian Esken <esken@kde.org>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "GlobalConfig.h"

// instanceObj must be created "late", so we can refer to the correct application config file kmixrc instead of kderc.
GlobalConfig* GlobalConfig::instanceObj;

GlobalConfig::GlobalConfig() :
	KConfigSkeleton()
{
	setCurrentGroup("Global");
	// General
	addItemBool("Tickmarks", data.showTicks, true);
	addItemBool("Labels", data.showLabels, true);
	addItemBool("VolumeOverdrive", data.volumeOverdrive, false);
	addItemBool("VolumeFeedback", data.beepOnVolumeChange, true);
	ItemString* is = addItemString("Orientation", data.orientationMainGUIString, "Vertical");
	kDebug() << is->name() << is->value();
	addItemString("Orientation.TrayPopup", data.orientationTrayPopupString, QLatin1String("Vertical"));

	// Sound Menu
	addItemBool("showOSD", data.showOSD, true);
	addItemBool("AllowDocking", data.showDockWidget, true);

//	addItemBool("TrayVolumeControl", data.trayVolumePopupEnabled, true); // removed support in KDE4.13. Always active!

	// Startup
	addItemBool("AutoStart", data.allowAutostart, true);
	addItemBool("VolumeFeedback", data.volumeFeedback, true);
	addItemBool("startkdeRestore", data.startkdeRestore, true);

	// Debug options: Not in dialog
	addItemBool("Debug.ControlManager", data.debugControlManager, false);
	addItemBool("Debug.GUI", data.debugGUI, false);
	addItemBool("Debug.Volume", data.debugVolume, false);

	readConfig();
}

// --- Special READ/WRITE ----------------------------------------------------------------------------------------
void GlobalConfig::usrReadConfig()
{
//	kDebug() << "or=" << data.orientationMainGUIString;
	// Convert orientation strings to Qt::Orientation
	data.convertOrientation();
}

//void GlobalConfig::usrWriteConfig()
//{
//	// TODO: Is this any good? When is usrWriteConfig() called? Hopefully BEFORE actually writing. Otherwise
//	//       I must move this code to #setToplevelOrientation() and #setTraypopupOrientation().
//}

Qt::Orientation GlobalConfigData::getToplevelOrientation()
{
	return toplevelOrientation;
}

Qt::Orientation GlobalConfigData::getTraypopupOrientation()
{
	return traypopupOrientation;
}

/**
 * Converts the orientation strings to Qt::Orientation
 */
void GlobalConfigData::convertOrientation()
{
	toplevelOrientation = stringToOrientation(orientationMainGUIString);
	traypopupOrientation = stringToOrientation(orientationTrayPopupString);
}

void GlobalConfigData::setToplevelOrientation(Qt::Orientation orientation)
{
	toplevelOrientation = orientation;
	orientationMainGUIString = orientationToString(toplevelOrientation);
}

void GlobalConfigData::setTraypopupOrientation(Qt::Orientation orientation)
{
	traypopupOrientation = orientation;
	orientationTrayPopupString = orientationToString(traypopupOrientation);
}

Qt::Orientation GlobalConfigData::stringToOrientation(QString& orientationString)
{
	return orientationString == "Horizontal" ? Qt::Horizontal : Qt::Vertical;
}

QString GlobalConfigData::orientationToString(Qt::Orientation orientation)
{
	return orientation == Qt::Horizontal ? "Horizontal" : "Vertical";
}
