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

#include "gui/guiprofile.h"

// Qt
#include <QDir>
#include <qxml.h>
#include <QString>

// System
#include <iostream>
#include <utility>

// KDE
#include <kdebug.h>
#include <kstandarddirs.h>

// KMix
#include "core/mixer.h"
#include <QtCore/qstring.h>

QMap<QString, GUIProfile*> GUIProfile::s_profiles;

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
    _dirty = false;
}

GUIProfile::~GUIProfile()
{
    kError() << "Thou shalt not delete any GUI profile. This message is only OK, when quitting KMix"; 
    qDeleteAll(_controls);
    qDeleteAll(_products);
}


void GUIProfile::setId(const QString& id)
{
    _id = id;
}

QString GUIProfile::getId() const
{
    return _id;
}

bool GUIProfile::isDirty()  const {
    return _dirty;
}

void GUIProfile::setDirty() {
    _dirty = true;
}

/**
 * Build a profile name. Suitable to use as primary key and to build filenames.
 * @arg mixer         The mixer
 * @arg profileName   The profile name (e.g. "capture", "playback", "my-cool-profile", or "any"
 * @return            The profile name
 */
QString GUIProfile::buildProfileName(Mixer* mixer, QString profileName, bool ignoreCard)
{
    QString fname;
    fname += mixer->getDriverName();
    if (!ignoreCard) {
        fname += ".%1.%2";
        fname = fname.arg(mixer->getBaseName()).arg(mixer->getCardInstance());
    }
    fname += '.' + profileName;

    fname.replace(' ','_');
    return fname;
}

/**
 * Generate a readable profile name (for presenting to the user).
 * Hint: Currently used as Tab label.
 */
QString GUIProfile::buildReadableProfileName(Mixer* mixer, QString profileName)
{
    QString fname;
    fname += mixer->getBaseName();
    if ( mixer->getCardInstance() > 1 ) {
        fname += ' ' + mixer->getCardInstance();
    }
    if ( profileName != "default" ) {
        fname += ' ' + profileName;
    }

    return fname;
}

/**
 * Returns the GUIProfile for the given ID (= "fullyQualifiedName").
 * If not found 0 is returned. There is no try to load it.
 * 
 * @returns The loaded GUIProfile for the given ID
 */
GUIProfile* GUIProfile::find(QString id)
{
  // Not thread safe (due to non-atomic contains()/get()
  if ( s_profiles.contains(id) )
  {
    return s_profiles[id];
  }
  else
  {
    return 0;
  }
}


/**
 * Finds the correct profile for the given mixer.
 * If already loaded from disk, returns the cached version.
 * Otherwise load profile from disk: Priority: Card specific profile, Card unspecific profile
 *
 * @arg mixer         The mixer
 * @arg profileName   The profile name (e.g. "ALSA.X-Fi.default", or "OSS.intel-cha51.playback")
 *                    A special case is "", which means that a card specific name should be generated.
 * @arg profileNameIsFullyQualified If true, an exact match will be searched. Otherwise it is a simple name like "playback" or "capture"
 * @arg ignoreCardName If profileName not fully qualified, this is used in building the requestedProfileName
 * @return GUIProfile*  The loaded GUIProfile, or 0 if no profile matched. Hint: if you use allowFallback==true, this should never return 0.
 */
GUIProfile* GUIProfile::find(Mixer* mixer, QString profileName, bool profileNameIsFullyQualified, bool ignoreCardName)
{
    GUIProfile* guiprof = 0;

    if ( mixer == 0 || profileName.isEmpty() )
        return 0;

    if ( mixer->isDynamic() ) {
        kDebug(67100) << "GUIProfile::find() Not loading GUIProfile for Dynamic Mixer (e.g. PulseAudio)";
        return 0;
    }

    QString requestedProfileName;
    QString fullQualifiedProfileName;
    if ( profileNameIsFullyQualified ) {
        requestedProfileName     = profileName;
        fullQualifiedProfileName = profileName;
    }
    else {
        requestedProfileName     = buildProfileName(mixer, profileName, ignoreCardName);
        fullQualifiedProfileName = buildProfileName(mixer, profileName, false);
    }

    if ( s_profiles.contains(fullQualifiedProfileName) ) {
        guiprof = s_profiles.value(fullQualifiedProfileName);  // Cached
    }
    else {
        guiprof = loadProfileFromXMLfiles(mixer, requestedProfileName);  // Load from XML ###Card specific profile###
        if ( guiprof != 0 ) {
            guiprof->_mixerId = mixer->id();
            guiprof->setId(fullQualifiedProfileName); // this one contains some soundcard id (basename + instance)

            if ( guiprof->getName().isEmpty() ) {
                // If the profile didn't contain a name then lets define one
                guiprof->setName(buildReadableProfileName(mixer,profileName)); // The caller can rename this if he likes
                guiprof->setDirty();
            }

            if ( requestedProfileName != fullQualifiedProfileName) {
                // This is very important!
                // When the final profileName (fullQualifiedProfileName) is different from
                // what we have loaded (requestedProfileName, e.g. "default"), we MUST
                // set the profile dirty, so it gets saved. Otherwise we would write the
                // fullQualifiedProfileName in the kmixrc, and will not find it on the next
                // start of KMix.
                guiprof->setDirty();
            }
            addProfile(guiprof);
        }
    }
    
    return guiprof;
}

/*
 * Add the profile to the internal list of profiles (Profile caching).
 */
void GUIProfile::addProfile(GUIProfile* guiprof)
{
    s_profiles[guiprof->getId()] = guiprof;
    kDebug() << "I have added" << guiprof->getId() << "; Number of profiles is now " <<  s_profiles.size() ;
    
}




/**
 * Loads a GUI Profile from disk (xml profile file).
 * It tries to load the Soundcard specific file first (a).
 * If it doesn't exist, it will load the default profile corresponding to the soundcard driver (b).
 */
GUIProfile* GUIProfile::loadProfileFromXMLfiles(Mixer* mixer, QString profileName)
{
    GUIProfile* guiprof = 0;
    QString fileName, fileNameFQ;

    fileName = "profiles/" + profileName + ".xml";
    fileNameFQ = KStandardDirs::locate("appdata", fileName );

    if ( ! fileNameFQ.isEmpty() ) {
        guiprof = new GUIProfile();
        if ( guiprof->readProfile(fileNameFQ) && ( guiprof->match(mixer) > 0) ) {
            // loaded
        }
        else {
            delete guiprof; // not good (e.g. Parsing error => drop this profile silently)
            guiprof = 0;
        }
    }
    else {
        kDebug() << "Ignore file " <<fileName<< " (does not exist)";
    }
    return guiprof;
}

/**
 * This is for all backends that ship no profile files
 */
GUIProfile* GUIProfile::fallbackProfile(Mixer *mixer)
{
    QString fullQualifiedProfileName = buildProfileName(mixer, QString("default"), false);

    GUIProfile *fallback = new GUIProfile();

    ProfProduct* prd = new ProfProduct();
    prd->vendor         = mixer->getDriverName();
    prd->productName    = mixer->readableName();
    prd->productRelease = "1.0";
    fallback->_products.insert(prd);

    static QString matchAll(".*");
    static QString matchAllSctl(".*");
    ProfControl* ctl = new ProfControl(matchAll, matchAllSctl);
    //ctl->regexp      = matchAll;   // make sure id matches the regexp
    ctl->setMandatory(true);
    fallback->_controls.push_back(ctl);

    fallback->_soundcardDriver = mixer->getDriverName();
    fallback->_soundcardName   = mixer->readableName();

    fallback->finalizeProfile();

    fallback->_mixerId = mixer->id();
    fallback->setId(fullQualifiedProfileName); // this one contains some soundcard id (basename + instance)
    fallback->setName(buildReadableProfileName(mixer, QString("default"))); // The caller can rename this if he likes
    fallback->setDirty();
    addProfile(fallback);

    return fallback;
}


/**
 * Fill the profile with the data from the given XML profile file.
 * @par  ref_fileName: Full qualified filename (with path).
 * @return bool True, if the profile was succesfully created. False if not (e.g. parsing error).
 */
bool GUIProfile::readProfile(const QString& ref_fileName)
{
    QXmlSimpleReader *xmlReader = new QXmlSimpleReader();
    kDebug() << "Read profile:" << ref_fileName ;
    QFile xmlFile( ref_fileName );
    QXmlInputSource source( &xmlFile );
    GUIProfileParser* gpp = new GUIProfileParser(this);
    xmlReader->setContentHandler(gpp);
    bool ok = xmlReader->parse( source );

    //std::cout << "Raw Profile: " << *this;
    if ( ok ) {
        ok = finalizeProfile();
    } // Read OK
    else {
        // !! this error message about faulty profiles should probably be surrounded with i18n()
        kError(67100) << "ERROR: The profile '" << ref_fileName<< "' contains errors, and is not used." << endl;
    }
    delete gpp;
    delete xmlReader;
   
    return ok;
}


bool GUIProfile::writeProfile()
{
   bool ret = false;
   QString fileName, fileNameFQ;
   fileName = "profiles/" + getId() + ".xml";
   fileName.replace(':', '.');
   fileNameFQ = KStandardDirs::locateLocal("appdata", fileName, true );

   kDebug() << "Write profile:" << fileNameFQ ;
   QFile f(fileNameFQ);
   if ( f.open(QIODevice::WriteOnly | QFile::Truncate) )
   { 
      QTextStream out(&f);
      out << *this;
      f.close();
      ret = true;
   }

   if ( ret ) {
       _dirty = false;
   }
   return ret;
}

/** This is now empty. It can be removed */
bool GUIProfile::finalizeProfile() const
{
    bool ok = true;
    return ok;
}

void GUIProfile::setControls(ControlSet& newControlSet)
{
    qDeleteAll(_controls);
    _controls = newControlSet;
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
	else if ( _soundcardName != mixer->getBaseName() ) {
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

QString xmlify(QString raw);

QString xmlify(QString raw)
{
// 	kDebug() << "Before: " << raw;
	raw = raw.replace('&', "&amp;");
	raw = raw.replace('<', "&lt;");
	raw = raw.replace('>', "&gt;");
	raw = raw.replace("'", "&apos;");
	raw = raw.replace("\"", "&quot;");
// 	kDebug() << "After : " << raw;
	return raw;
}


QTextStream& operator<<(QTextStream &os, const GUIProfile& guiprof)
{
// kDebug() << "ENTER QTextStream& operator<<";
   os << "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
   os << endl << endl;

   os << "<soundcard driver=\"" << xmlify(guiprof._soundcardDriver).toUtf8().constData() << "\""
      << " version = \"" << guiprof._driverVersionMin << ":" << guiprof._driverVersionMax  << "\"" << endl
      << " name = \"" << xmlify(guiprof._soundcardName).toUtf8().constData() << "\"" << endl
      << " type = \"" << xmlify(guiprof._soundcardType).toUtf8().constData() << "\"" << endl
      << " generation = \"" << guiprof._generation << "\"" << endl
      << ">" << endl << endl ;

   os << "<profile id=\"" << xmlify(guiprof._id) << "\" name=\"" << xmlify(guiprof._name) << "\"/>" << endl;

   for ( GUIProfile::ProductSet::const_iterator it = guiprof._products.begin(); it != guiprof._products.end(); ++it)
	{
		ProfProduct* prd = *it;
		os << "<product vendor=\"" << xmlify(prd->vendor).toUtf8().constData() << "\" name=\"" << xmlify(prd->productName).toUtf8().constData() << "\"";
		if ( ! prd->productRelease.isNull() ) {
			os << " release=\"" << xmlify(prd->productRelease).toUtf8().constData() << "\"";
		}
		if ( ! prd->comment.isNull() ) {
			os << " comment=\"" << xmlify(prd->comment).toUtf8().constData() << "\"";
		}
		os << " />" << endl;
	} // for all products
	os << endl;

    foreach ( ProfControl* profControl, guiprof.getControls() )
	{
		os << "<control id=\"" << xmlify(profControl->id).toUtf8().constData() << "\"" ;
		if ( !profControl->name.isNull() && profControl->name != profControl->id ) {
		  os << " name=\"" << xmlify(profControl->name).toUtf8().constData() << "\"" ;
		}
		os << " subcontrols=\"" << xmlify( profControl->renderSubcontrols().toUtf8().constData()) << "\"" ;
		os << " show=\"" << xmlify(profControl->show).toUtf8().constData() << "\"" ;
		if ( profControl->isMandatory() ) {
		  os << " mandatory=\"true\"";
		}
		if ( profControl->isSplit() ) {
		  os << " split=\"true\"";
		}
		os << " />" << endl;
	} // for all controls
	os << endl;

	os << "</soundcard>" << endl;
// kDebug() << "EXIT  QTextStream& operator<<";
	return os;
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

    os << "Profile:" << std::endl
            << "    Id=" << guiprof._id.toUtf8().constData() << std::endl
            << "  Name=" << guiprof._name.toUtf8().constData() << std::endl;

	for ( GUIProfile::ProductSet::const_iterator it = guiprof._products.begin(); it != guiprof._products.end(); ++it)
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

    foreach ( ProfControl* profControl, guiprof.getControls() )
	{
//		ProfControl* profControl = *it;
		os << "Control:\n  ID=" << profControl->id.toUtf8().constData() << std::endl;
		if ( !profControl->name.isNull() && profControl->name != profControl->id ) {
		 		os << "  Name = " << profControl->name.toUtf8().constData() << std::endl;
		}
		os << "  Subcontrols=" << profControl->renderSubcontrols().toUtf8().constData() << std::endl;
		if ( profControl->isMandatory() ) {
		    os << " mandatory=\"true\"" << std::endl;
		}
		if ( profControl->isSplit() ) {
		    os << " split=\"true\"" << std::endl;
		}
	} // for all controls

	return os;
}

ProfControl::ProfControl(QString& id, QString& subcontrols ) :
	  _mandatory(false), _split(false) {
    d = new ProfControlPrivate();
    this->show = "simple";
    this->id = id;
    setSubcontrols(subcontrols);
}

ProfControl::ProfControl(const ProfControl &profControl) :
	  _mandatory(false), _split(false) {
    d = new ProfControlPrivate();
    id = profControl.id;
    name = profControl.name;

    _useSubcontrolPlayback = profControl._useSubcontrolPlayback;
    _useSubcontrolCapture = profControl._useSubcontrolCapture;
    _useSubcontrolPlaybackSwitch = profControl._useSubcontrolPlaybackSwitch;
    _useSubcontrolCaptureSwitch = profControl._useSubcontrolCaptureSwitch;
    _useSubcontrolEnum = profControl._useSubcontrolEnum;
    d->subcontrols = profControl.d->subcontrols;

    name = profControl.name;
    show = profControl.show;
    backgroundColor = profControl.backgroundColor;
    switchtype = profControl.switchtype;
    _mandatory = profControl._mandatory;
    _split = profControl._split;
}

ProfControl::~ProfControl() {
    delete d;
}

void ProfControl::setSubcontrols(QString sctls)
{
    d->subcontrols = sctls;

  _useSubcontrolPlayback = false;
  _useSubcontrolCapture = false;
  _useSubcontrolPlaybackSwitch = false;
  _useSubcontrolCaptureSwitch = false;
  _useSubcontrolEnum = false;

  QStringList qsl = sctls.split( ',',  QString::SkipEmptyParts, Qt::CaseInsensitive);
  QStringListIterator qslIt(qsl);
  while (qslIt.hasNext()) {
    QString sctl = qslIt.next();
       //kDebug() << "setSubcontrols found: " << sctl.toLocal8Bit().constData() << endl;
       if ( sctl == "pvolume" ) _useSubcontrolPlayback = true;
       else if ( sctl == "cvolume" ) _useSubcontrolCapture = true;
       else if ( sctl == "pswitch" ) _useSubcontrolPlaybackSwitch = true;
       else if ( sctl == "cswitch" ) _useSubcontrolCaptureSwitch = true;
       else if ( sctl == "enum" ) _useSubcontrolEnum = true;
       else if ( sctl == "*" || sctl == ".*") {
	 _useSubcontrolCapture = true;
	 _useSubcontrolCaptureSwitch = true;
	 _useSubcontrolPlayback = true;
	 _useSubcontrolPlaybackSwitch = true;
	 _useSubcontrolEnum = true;
       }
       else kWarning() << "Ignoring unknown subcontrol type '" << sctl << "' in profile";
  }
}

QString ProfControl::renderSubcontrols()
{
    QString sctlString;
    if ( _useSubcontrolPlayback && _useSubcontrolPlaybackSwitch && _useSubcontrolCapture && _useSubcontrolCaptureSwitch && _useSubcontrolEnum ) {
        return QString("*");
    }
    else {
        if ( _useSubcontrolPlayback ) {
            sctlString += "pvolume,";
        }
        if ( _useSubcontrolCapture ) {
            sctlString += "cvolume,";
        }
        if ( _useSubcontrolPlaybackSwitch ) {
            sctlString += "pswitch,";
        }
        if ( _useSubcontrolCaptureSwitch ) {
            sctlString += "cswitch,";
        }
        if ( _useSubcontrolEnum ) {
            sctlString += "enum,";
        }
        if ( sctlString.length() > 0 ) {
            sctlString.chop(1);
        }
        return sctlString;
    }
}


// ### PARSER START ################################################


GUIProfileParser::GUIProfileParser(GUIProfile* ref_gp) : _guiProfile(ref_gp)
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
            else if ( qName.toLower() == "profile" ) {
                addProfileInfo(attributes);
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
		_guiProfile->_soundcardDriver = driver;
		_guiProfile->_soundcardName = name;
		if ( type.isNull() ) {
			_guiProfile->_soundcardType = "";
		}
		else {
			_guiProfile->_soundcardType = type;
		}
		if ( version.isNull() ) {
			_guiProfile->_driverVersionMin = 0;
			_guiProfile->_driverVersionMax = 0;
		}
		else {
			std::pair<QString,QString> versionMinMax;
			splitPair(version, versionMinMax, ':');
			_guiProfile->_driverVersionMin = versionMinMax.first.toULong();
			_guiProfile->_driverVersionMax = versionMinMax.second.toULong();
		}
		if ( type.isNull() ) { type = ""; };
		if ( generation.isNull() ) {
			_guiProfile->_generation = 0;
		}
		else {
			// Hint: If the conversion fails, _generation will be assigned 0 (which is fine)
			_guiProfile->_generation = generation.toUInt();
		}
	}

}



void GUIProfileParser::addProfileInfo(const QXmlAttributes& attributes) {
    QString name = attributes.value("name");
    QString id   = attributes.value("id");

    _guiProfile->setId(id);
    _guiProfile->setName(name);
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

		_guiProfile->_products.insert(prd);
	}
}

void GUIProfileParser::addControl(const QXmlAttributes& attributes) {
    /*
    std::cout  << "Control: ";
    printAttributes(attributes);
    */
    QString id = attributes.value("id");
    QString subcontrols = attributes.value("subcontrols");
    QString name = attributes.value("name");
    QString show = attributes.value("show");
    QString background = attributes.value("background");
    QString switchtype = attributes.value("switchtype");
    QString mandatory = attributes.value("mandatory");
    QString split = attributes.value("split");
    bool isMandatory = false;

    if ( !id.isNull() ) {
        // We need at least an "id". We can set defaults for the rest, if undefined.
        if ( subcontrols.isNull() || subcontrols.isEmpty() ) {
            subcontrols = '*';  // for compatibility reasons, we interpret an empty string as match-all (aka "*")
        }
        if ( name.isNull() ) {
            // ignore. isNull() will be checked by all users.
        }
        if ( ! mandatory.isNull() && mandatory == "true" ) {
            isMandatory = true;
        }
        if ( !background.isNull() ) {
            // ignore. isNull() will be checked by all users.
        }
        if ( !switchtype.isNull() ) {
            // ignore. isNull() will be checked by all users.
        }

        ProfControl *profControl = new ProfControl(id, subcontrols);
        if ( show.isNull() ) { show = '*'; }

	profControl->name = name;
	profControl->show = show;
	profControl->setBackgroundColor( background );
	profControl->setSwitchtype(switchtype);
	profControl->setMandatory(isMandatory);
	
        if ( !split.isNull() && split=="true") {
	    profControl->setSplit(true);
	}
	_guiProfile->getControls().push_back(profControl);
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
		result.first  = pairString.mid(0,delimPos);
		result.second = pairString.left(delimPos+1);
	}
}
