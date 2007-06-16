/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2006-2007 Christian Esken <esken@kde.org>
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

#include "guiprofile.h"

// Qt
#include <qxml.h>
#include <QString>

// System
#include <iostream>
#include <utility>

// KDE
#include <kdebug.h>

// KMix
#include "mixer.h"


bool SortedStringComparator::operator()(const std::string& s1, const std::string& s2) const {
    return ( s1 < s2 );
}

/**
 * Product comparator for sorting:
 * We want the comparator to sort ascending by Vendor. "Inside" the Vendors, we sort by Product Name.
 */
bool ProductComparator::operator()(const ProfProduct* p1, const ProfProduct* p2) const {
	if ( p1->vendor < p2->vendor ) {
		return ( true );
	}
	else if ( p1->vendor > p2->vendor ) {
		return ( false );
	}
	else if ( p1->productName < p2->productName ) {
		return ( true );
	}
	else if ( p1->productName > p2->productName ) {
		return ( false );
	}
	else {
		/**
		 * We reach this point, if vendor and product name is identical.
		 * Actually we don't care about the order then, so we decide that "p1" comes first.
		 *
		 * (Hint: As this is a set comparator, the return value HERE doesn't matter that
		 * much. But if we would decide later to change this Comparator to be a Map Comparator,
		 *  we must NOT return a "0" for identity - this would lead to non-insertion on insert())
		 */
		return true;
	}
}

GUIProfile::GUIProfile()
{
}

GUIProfile::~GUIProfile()
{
	// !!! must delete all   std::map and std::set  content and instances here.
	//    QString content is reference counted and probably doesn't need to be deleted explicitly.
}

bool GUIProfile::readProfile(QString& ref_fileName)
{
	QXmlSimpleReader *xmlReader = new QXmlSimpleReader();

	QFile xmlFile( ref_fileName );
	QXmlInputSource source( &xmlFile );
	GUIProfileParser* gpp = new GUIProfileParser(*this);
	xmlReader->setContentHandler(gpp);
	bool ok = xmlReader->parse( source );
	delete gpp;

        //std::cout << "Raw Profile: " << *this;
	if ( ok ) {
		// Reading is OK => now make the profile consistent

		// (1) Make sure the _tabs are complete (add any missing Tabs)
		std::vector<ProfControl*>::const_iterator itEnd = _controls.end();
		for ( std::vector<ProfControl*>::const_iterator it = _controls.begin(); it != itEnd; ++it)
		{
			ProfControl* control = *it;
			QString tabnameOfControl = control->tab;
			if ( tabnameOfControl.isNull() ) {
				// OK, it has no TabName defined. We will assign a TabName in step (3).
			}
			else {
				// check, whether we have this Tab yet.
				//std::vector<ProfTab*>::iterator tabRef = std::find(_tabs.begin(), _tabs.end(), tabnameOfControl);
				std::vector<ProfTab*>::const_iterator itTEnd = _tabs.end();
				std::vector<ProfTab*>::const_iterator itT = _tabs.begin();
				for ( ; itT != itTEnd; ++itT) {
				    if ( (*itT)->name == tabnameOfControl ) break;
				}
				if ( itT == itTEnd ) {
					// no such Tab yet => insert it
					ProfTab* tab = new ProfTab();
					tab->name = tabnameOfControl;
					tab->type = "SliderSet";  //  as long as we don't know better
					_tabs.push_back(tab);
				} // tab does not exist yet => insert new tab
			} // Control contains a TabName
		} // Step (1)

		// (2) Make sure that there is at least one Tab
		if ( _tabs.size() == 0) {
			ProfTab* tab = new ProfTab();
			tab->name = "Controls"; // !! A better name should be used. What about i18n() ?
			tab->type = "SliderSet";  //  as long as we don't know better
			_tabs.push_back(tab);
		} // Step (2)

		// (3) Assign a Tab Name to all controls that have no defined Tab Name yet.
		ProfTab* tab = _tabs.front();
		itEnd = _controls.end();		for ( std::vector<ProfControl*>::const_iterator it = _controls.begin(); it != itEnd; ++it)
		{
			ProfControl* control = *it;
			QString& tabnameOfControl = control->tab;
			if ( tabnameOfControl.isNull() ) {
				// OK, it has no TabName defined. We will assign a TabName in step (3).
				control->tab = tab->name;
			}
		} // Step (3)
		//std::cout << "Consistent Profile: " << *this;

	} // Read OK
	else {
		// !! this error message about faulty profiles should probably be surrounded with i18n()
		kError(67100) << "ERROR: The profile '" << ref_fileName<< "' contains errors, and is not used." << endl;
	}

	return ok;
}

/**
 * Returns how good the given Mixer matches this GUIProfile.
 * A value between 0 (not matching at all) and MAXLONG (perfect match) is returned.
 *
 * Here is the current algorithm:
 *
 * If the driver doesn't match, 0 is returned. (OK)
 * If the card-name ...  (OK)
 *     is "*", this is worth 1 point
 *     doesn't match, 0 is returned.
 *     matches, this is worth 500 points.
 *
 * If the "card type" ...
 *     is empty, this is worth 0 points.     !!! not implemented yet
 *     doesn't match, 0 is returned.         !!! not implemented yet
 *     matches , this is worth 500 points.  !!! not implemented yet
 *
 * If the "driver version" doesn't match, 0 is returned. !!! not implemented yet
 * If the "driver version" matches, this is worth ...
 *     4000 unlimited                             <=> "*:*"
 *     6000 toLower-bound-limited                   <=> "toLower-bound:*"
 *     6000 upper-bound-limited                   <=> "*:upper-bound"
 *     8000 upper- and toLower-bound limited        <=> "toLower-bound:upper-bound"
 * or 10000 points (upper-bound=toLower-bound=bound <=> "bound:bound"
 *
 * The Profile-Generation is added to the already achieved points. (done)
 *   The maximum gain is 900 points.
 *   Thus you can create up to 900 generations (0-899) without "overriding"
 *   the points gained from the "driver version" or "card-type".
 *
 * For example:  card-name="*" (1), card-type matches (1000),
 *               driver version "*:*" (4000), Profile-Generation 4 (4).
 *         Sum:  1 + 1000 + 4000 + 4 = 5004
 *
 * @todo Implement "card type" match value
 * @todo Implement "version" match value (must be in backends as well)
 */
unsigned long GUIProfile::match(Mixer* mixer) {
	unsigned long matchValue = 0;
	if ( _soundcardDriver != mixer->getDriverName() ) {
		return 0;
	}
	if ( _soundcardName == "*" ) {
		matchValue += 1;
	}
	else if ( _soundcardName != mixer->baseName() ) {
		return 0; // card name does not match
	}
	else {
		matchValue += 500; // card name matches
	}

	// !!! we don't check current for the driver version.
	//     So we assign simply 4000 points for now.
	matchValue += 4000;
	if ( _generation < 900 ) {
		matchValue += _generation;
	}
	else {
		matchValue += 900;
	}
	return matchValue;
}

std::ostream& operator<<(std::ostream& os, const GUIProfile& guiprof) {
	os  << "Soundcard:" << std::endl
			<< "  Driver=" << guiprof._soundcardDriver.toUtf8().constData() << std::endl
			<< "  Driver-Version min=" << guiprof._driverVersionMin
			<< " max=" << guiprof._driverVersionMax << std::endl
			<< "  Card-Name=" << guiprof._soundcardName.toUtf8().constData() << std::endl
			<< "  Card-Type=" << guiprof._soundcardType.toUtf8().constData() << std::endl
			<< "  Profile-Generation="  << guiprof._generation
			<< std::endl;
	for ( std::set<ProfProduct*>::iterator it = guiprof._products.begin(); it != guiprof._products.end(); ++it)
	{
		ProfProduct* prd = *it;
		os << "Product:\n  Vendor=" << prd->vendor.toUtf8().constData() << std::endl << "  Name=" << prd->productName.toUtf8().constData() << std::endl;
		if ( ! prd->productRelease.isNull() ) {
			os << "  Release=" << prd->productRelease.toUtf8().constData() << std::endl;
		}
		if ( ! prd->comment.isNull() ) {
			os << "  Comment = " << prd->comment.toUtf8().constData() << std::endl;
		}
	} // for all products

	for ( std::vector<ProfTab*>::const_iterator it = guiprof._tabs.begin(); it != guiprof._tabs.end(); ++it) {
		ProfTab* profTab = *it;
		os << "Tab: " << std::endl << "  " << profTab->name.toUtf8().constData() << " (" << profTab->type.toUtf8().constData() << ")" << std::endl;
	} // for all tabs

	for ( std::vector<ProfControl*>::const_iterator it = guiprof._controls.begin(); it != guiprof._controls.end(); ++it)
	{
		ProfControl* profControl = *it;
		os << "Control:\n  ID=" << profControl->id.toUtf8().constData() << std::endl;
		if ( !profControl->name.isNull() && profControl->name != profControl->id ) {
		 		os << "  Name = " << profControl->name.toUtf8().constData() << std::endl;
		}
		os << "  Subcontrols=" << profControl->subcontrols.toUtf8().constData() << std::endl;
		if ( ! profControl->tab.isNull() ) {
			os << "  Tab=" << profControl->tab.toUtf8().constData() << std::endl;
		}
		os << "  Shown-On=" << profControl->show.toUtf8().constData() << std::endl;
	} // for all controls

	return os;
}




// ### PARSER START ################################################


GUIProfileParser::GUIProfileParser(GUIProfile& ref_gp) : _guiProfile(ref_gp)
{
}

bool GUIProfileParser::startDocument()
{
	_scope = GUIProfileParser::NONE;  // no scope yet
	return true;
}

bool GUIProfileParser::startElement( const QString& ,
                                    const QString& ,
                                    const QString& qName,
                                    const QXmlAttributes& attributes )
{
	switch ( _scope ) {
		case GUIProfileParser::NONE:
			/** we are reading the "top level" ***************************/
			if ( qName.toLower() == "soundcard" ) {
				_scope = GUIProfileParser::SOUNDCARD;
				addSoundcard(attributes);
			}
			else {
				// skip unknown top-level nodes
				std::cerr << "Ignoring unsupported element '" << qName.toUtf8().constData() << "'" << std::endl;
			}
			// we are accepting <soundcard> only
		break;

		case GUIProfileParser::SOUNDCARD:
			if ( qName.toLower() == "product" ) {
				// Defines product names under which the chipset/hardware is sold
				addProduct(attributes);
			}
			else if ( qName.toLower() == "control" ) {
				addControl(attributes);
			}
			else if ( qName.toLower() == "tab" ) {
				addTab(attributes);
			}
			else {
				std::cerr << "Ignoring unsupported element '" << qName.toUtf8().constData() << "'" << std::endl;
			}
			// we are accepting <product>, <control> and <tab>

		break;

	} // switch()
    return true;
}

bool GUIProfileParser::endElement( const QString&, const QString&, const QString& qName )
{
	if ( qName == "soundcard" ) {
		_scope = GUIProfileParser::NONE; // should work out OK, as we don't nest soundcard entries
	}
    return true;
}

void GUIProfileParser::addSoundcard(const QXmlAttributes& attributes) {
/*
	std::cout  << "Soundcard: ";
	printAttributes(attributes);
*/
	QString driver	= attributes.value("driver");
	QString version = attributes.value("version");
	QString name	= attributes.value("name");
	QString type	= attributes.value("type");
	QString generation = attributes.value("generation");
	if ( !driver.isNull() && !name.isNull() ) {
		_guiProfile._soundcardDriver = driver;
		_guiProfile._soundcardName = name;
		if ( type.isNull() ) {
			_guiProfile._soundcardType = "";
		}
		else {
			_guiProfile._soundcardType = type;
		}
		if ( version.isNull() ) {
			_guiProfile._driverVersionMin = 0;
			_guiProfile._driverVersionMax = 0;
		}
		else {
			std::pair<QString,QString> versionMinMax;
			splitPair(version, versionMinMax, ':');
			_guiProfile._driverVersionMin = versionMinMax.first.toULong();
			_guiProfile._driverVersionMax = versionMinMax.second.toULong();
		}
		if ( type.isNull() ) { type = ""; };
		if ( generation.isNull() ) {
			_guiProfile._generation = 0;
		}
		else {
			// Hint: If the conversion fails, _generation will be assigned 0 (which is fine)
			_guiProfile._generation = generation.toUInt();
		}
	}

}


void GUIProfileParser::addTab(const QXmlAttributes& attributes) {
/*
	    	std::cout  << "Tab: ";
	    	printAttributes(attributes);
*/
	QString name = attributes.value("name");
	QString type	= attributes.value("type");
	if ( !name.isNull() && !type.isNull() ) {
		// If you define a Tab, you must set its Type
		// It is either "Input", "Output", "Switches" or "Surround"
		// These names are case sensitive and correspond 1:1 to the View-Names.
		// This could make it possible in the (far) future to have Views as Plugins.
		ProfTab* tab = new ProfTab();
		tab->name = name;
		tab->type = type;

		_guiProfile._tabs.push_back(tab);
	}
}

void GUIProfileParser::addProduct(const QXmlAttributes& attributes) {
	/*
	std::cout  << "Product: ";
	printAttributes(attributes);
	*/
	QString vendor = attributes.value("vendor");
	QString name = attributes.value("name");
	QString release = attributes.value("release");
	QString comment = attributes.value("comment");
	if ( !vendor.isNull() && !name.isNull() ) {
		// Adding a product makes only sense if we have at least vendor and product name
		ProfProduct *prd = new ProfProduct();
		prd->vendor = vendor;
		prd->productName = name;
		prd->productRelease = release;
		prd->comment = comment;

		_guiProfile._products.insert(prd);
	}
}

void GUIProfileParser::addControl(const QXmlAttributes& attributes) {
	/*
	std::cout  << "Control: ";
	printAttributes(attributes);
	*/
	QString id = attributes.value("id");
	QString subcontrols = attributes.value("controls");
	QString tab = attributes.value("tab");
	QString name = attributes.value("name");
   QString regexp = attributes.value("pattern");
	QString show = attributes.value("show");
	if ( !id.isNull() ) {
		// We need at least an "id". We can set defaults for the rest, if undefined.
		ProfControl *profControl = new ProfControl();
		if ( subcontrols.isNull() ) {
			subcontrols = "*";
		}
		if ( tab.isNull() ) {
			// Ignore this for the moment. We will put it on the first existing Tab at the end of parsing
		}
		if ( name.isNull() ) {
         // ignore. isNull() will be checked by all users.
		}
      if ( regexp.isNull() ) {
         // !! should do a dictonary lookup here, and i18n(). For now, just take over "id"
         regexp = !name.isNull() ? name : id;
      }

		profControl->id = id;
		profControl->name = name;
		profControl->subcontrols = subcontrols;
		profControl->name = name;
		profControl->tab = tab;
		if ( show.isNull() ) { show = "*"; }
		profControl->show = show;
		_guiProfile._controls.push_back(profControl);
	} // id != null
}

void GUIProfileParser::printAttributes(const QXmlAttributes& attributes) {
		    if ( attributes.length() > 0 ) {
		        for ( int i = 0 ; i < attributes.length(); i++ ) {
					std::cout << attributes.qName(i).toUtf8().constData() << ":"<< attributes.value(i).toUtf8().constData() << " , ";
		        }
			    std::cout << std::endl;
		    }
}

void GUIProfileParser::splitPair(const QString& pairString, std::pair<QString,QString>& result, char delim)
{
	int delimPos = pairString.indexOf(delim);
	if ( delimPos == -1 ) {
		// delimiter not found => use an empty String for "second"
		result.first  = pairString;
		result.second = "";
	}
	else {
		// delimiter found
		result.first  = pairString.mid(0,delimPos);      // check this !!!
		result.second = pairString.left(delimPos+1);
	}
}
