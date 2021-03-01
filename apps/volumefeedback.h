/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2021 Jonathan Marten <jjm@keelhaul.me.uk>
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
 * License along with this program; if not, see
 * <https://www.gnu.org/licenses>.
 */

#ifndef VOLUMEFEEDBACK_H
#define VOLUMEFEEDBACK_H


#include <qobject.h>

#include "core/ControlManager.h"


class QTimer;
class ca_context;


class VolumeFeedback : public QObject
{
	Q_OBJECT

public:
	static VolumeFeedback *instance();
	void init();

public slots:
	void controlsChange(ControlManager::ChangeType changeType);

private:
	VolumeFeedback();
	virtual ~VolumeFeedback();

	void volumeChanged();
	void masterChanged();

private slots:
	void slotPlayFeedback();

private:
	QString m_currentMaster;
	int m_currentVolume;
	QTimer *m_feedbackTimer;
	bool m_veryFirstTime;

	ca_context *m_ccontext;
};

#endif							// VOLUMEFEEDBACK_H
