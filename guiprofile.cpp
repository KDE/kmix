#include "guiprofile.h"

#include <qxml.h>
#include <qstring.h>

#include <iostream>

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
		 * (Hint: As this is a set comparator, the return value doesn't matter that
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
	//    QString content is reference counted and probably doesn't need to be deleted explicitely.
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
		
		return ok;
}


std::ostream& operator<<(std::ostream& os, const GUIProfile& guiprof) {
	for ( std::set<ProfProduct*>::iterator it = guiprof._products.begin(); it != guiprof._products.end();  it++)
	{
		ProfProduct* prd = *it;
		os << "Product:\n Vendor=" << prd->vendor.utf8() << std::endl << " Name=" << prd->productName.utf8() << std::endl;
		if ( ! prd->productRelease.isNull() ) {
			os << " Release=" << prd->productRelease.utf8()<< std::endl;
		}
		if ( ! prd->comment.isNull() ) {
			os << " Comment = " << prd->comment.utf8() << std::endl;
		}
	} // for all products

	for ( std::set<ProfControl*>::iterator it = guiprof._controls.begin(); it != guiprof._controls.end();  it++)
	{
		ProfControl* profControl = *it;
		os << "Control:\n ID=" << profControl->id.utf8() << std::endl;
		if ( profControl->name != profControl->id ) {
		 		os << " Name = " << profControl->name.utf8() << std::endl;
		}
		os << " Subcontrols=" << profControl->subcontrols.utf8() << std::endl;
		if ( ! profControl->tab.isNull() ) {
			os << " Tab=" << profControl->tab.utf8()<< std::endl;
		}
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
			if ( qName.lower() == "soundcard" ) {
				_scope = GUIProfileParser::SOUNDCARD;
				addSoundcard(attributes);
			}
			else {
				// skip unknown top-level nodes
				std::cerr << "Ignoring unsupported element '" << qName.ascii() << "'" << std::endl;
			}
			// we are accepting <soundcard> and <tab>
		break;
		
		case GUIProfileParser::SOUNDCARD:
			if ( qName.lower() == "product" ) {
				// Defines product names under which the chipset/hardware is sold
				addProduct(attributes);
			}
			else if ( qName.lower() == "control" ) {
				addControl(attributes);
			}
			else if ( qName.lower() == "tab" ) {
				addTab(attributes);
			}
			else {
				std::cerr << "Ignoring unsupported element '" << qName.ascii() << "'" << std::endl;
			}
			// we are accepting <soundcard> and <tab>
			
	    break;
	    
		case GUIProfileParser::TAB:
	    	std::cout  << "Tab: ";
		    if ( attributes.length() > 0 ) {
		        for ( int i = 0 ; i < attributes.length(); i++ ) {
					std::cout << attributes.qName(i).ascii() << ":"<< attributes.uri(i).ascii() << " , ";
		        }
		    }
		    std::cout << std::endl;
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
	    	std::cout  << "Soundcard: ";
	    	printAttributes(attributes);
}

void GUIProfileParser::addTab(const QXmlAttributes& attributes) {
	    	std::cout  << "Tab: ";
	    	printAttributes(attributes);
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
			// !! should do a dictonary lookup here, and i18n(). For now, just take over "id"
			name = id;
		}
		
		profControl->id = id;
		profControl->name = name;
		profControl->subcontrols = subcontrols;
		profControl->name = name;
		
		_guiProfile._controls.insert(profControl);
	} // id != null
}

void GUIProfileParser::printAttributes(const QXmlAttributes& attributes) {
		    if ( attributes.length() > 0 ) {
		        for ( int i = 0 ; i < attributes.length(); i++ ) {
					std::cout << attributes.qName(i).utf8() << ":"<< attributes.value(i).utf8() << " , ";
		        }
			    std::cout << std::endl;
		    }
}
