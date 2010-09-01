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
		std::cout << "GuiProfile '" << qPrintable(fileName) << "' read succesfully:\n----------------------\n";
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
