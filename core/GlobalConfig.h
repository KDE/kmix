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

#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include <Qt>
#include <QSet>

#include <KConfigSkeleton>

#include "kmixcore_export.h"

class KMIXCORE_EXPORT GlobalConfigData
{
	friend class GlobalConfig;

public:
	// Hint: We are using the standard 1-arg constructor as copy constructor

	bool showTicks;
	bool showLabels;
	bool showOSD;

	bool volumeFeedback;

	bool volumeOverdrive; // whether more than recommended volume (typically 0dB) is allowed
	bool beepOnVolumeChange;

	// Startup
	bool allowAutostart;
	bool showDockWidget;
	bool startkdeRestore;

	// Debug options
	bool debugControlManager;
	bool debugGUI;
	bool debugVolume;
	bool debugConfig;

	Qt::Orientation getToplevelOrientation();
	Qt::Orientation getTraypopupOrientation();

	void setToplevelOrientation(Qt::Orientation orientation);
	void setTraypopupOrientation(Qt::Orientation orientation);

private:
	QString orientationMainGUIString;
	QString orientationTrayPopupString;
	// The following two values are only converted/cached date from the former fields.
	Qt::Orientation toplevelOrientation;
	Qt::Orientation traypopupOrientation;

	void convertOrientation();
	Qt::Orientation stringToOrientation(QString& orientationString);
	QString orientationToString(Qt::Orientation orientation);

};

class KMIXCORE_EXPORT GlobalConfig : public KConfigSkeleton
{
private:
	static GlobalConfig* instanceObj;

public:
	static GlobalConfig& instance()
	{
		return *instanceObj;
	}
	;

	/**
	 * Call this init method when your app core is properly initialized.
	 * It is very important that KGlobal is initialized then. Otherwise
	 * KSharedConfig::openConfig() could return a reference to
	 * the "kderc" config instead of the actual application config "kmixrc" or "kmixctrlrc".
	 *
	 */
	static void init()
	{
		instanceObj = new GlobalConfig();
	}
	;

	/**
	 * Frees all data associated with the static instance.
	 *
	 */
	static void shutdown()
	{
		delete instanceObj;
		instanceObj = 0;
	}
	;


	GlobalConfigData data;
	void setMixersForSoundmenu(QSet<QString> mixersForSoundmenu)
	{
		this->mixersForSoundmenu = mixersForSoundmenu;
	}
	;
	QSet<QString> getMixersForSoundmenu()
	{
		return mixersForSoundmenu;
	}
	;

protected:
	QSet<QString> mixersForSoundmenu;

private:

	GlobalConfig();
	/**
	 * @Override
	 */
	void usrReadConfig() Q_DECL_OVERRIDE;
	/**
	 * @Override
	 */
//	virtual void usrWriteConfig();
};

#endif // GLOBALCONFIG_H
