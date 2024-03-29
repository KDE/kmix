/*
 *              KMix -- KDE's full featured mini mixer
 *
 *              Copyright (C) 2005 Christian Esken
 *                        esken@kde.org
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

#include "profiletest.h"

#include <QTest>

#include "../gui/guiprofile.h"

void ProfileTest::testReadProfile()
{
	GUIProfile* guiprof = new GUIProfile();
	
	QString fileName = QStringLiteral("profiletest.xml");
		
	bool ok = guiprof->readProfile(fileName);
    QVERIFY(ok);
}

QTEST_GUILESS_MAIN(ProfileTest)
