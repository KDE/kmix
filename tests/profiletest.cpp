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

#include "../gui/guiprofile.h"

#include <iostream>

int main(int argc, char* argv[]) {
	GUIProfile* guiprof = new GUIProfile();
	
	char dummyFilename[] = "profiletest.xml";
	QString fileName;
	
	if ( argc >=1 && argv[1] != 0 ) {
		fileName = argv[1];
	}
	else {
		fileName = dummyFilename;
	}
		
	bool ok = guiprof->readProfile(fileName);
	if ( !ok ) {
		std::cerr << "Error: GuiProfile '" << qPrintable(fileName) << "' is NOT ok" << std::endl;
	}
	else {
		std::cout << "GuiProfile '" << qPrintable(fileName) << "' read successfully:\n----------------------\n";
		std::cout << (*guiprof) ;
		std::cout << "----------------------\n";
	}
	
	if ( ok ) {
		return 0;
	}
	else {
		return 1;
	}
}
