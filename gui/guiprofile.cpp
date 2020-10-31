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
#include <QSaveFile>
#include <QXmlStreamWriter>
#include <QXmlSimpleReader>
#include <QStandardPaths>

// KMix
#include "core/mixer.h"


QMap<QString, GUIProfile *> s_profiles;
const QString s_profileDir("profiles");


static QString visibilityToString(GuiVisibility vis)
{
	switch (vis)
	{
case GuiVisibility::Simple:	return ("simple");
case GuiVisibility::Extended:	return ("extended");
// For backwards compatibility, 'Full' has the ID "all" and not "full"
case GuiVisibility::Full:	return ("all");
case GuiVisibility::Custom:	return ("custom");
case GuiVisibility::Never:	return ("never");
case GuiVisibility::Default:	return ("default");
default:			return ("unknown");
	}
}


static GuiVisibility visibilityFromString(const QString &str)
{
	if (str=="simple") return (GuiVisibility::Simple);
	else if (str=="extended") return (GuiVisibility::Extended);
	else if (str=="all") return (GuiVisibility::Full);
	else if (str=="custom") return (GuiVisibility::Custom);
	else if (str=="never") return (GuiVisibility::Never);

	qCWarning(KMIX_LOG) << "Unknown string value" << str;
	return (GuiVisibility::Full);
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
    _driverVersionMin = 0;
    _driverVersionMax = 0;
    _generation = 1;
}

GUIProfile::~GUIProfile()
{
    qCWarning(KMIX_LOG) << "Thou shalt not delete any GUI profile. This message is only OK, when quitting KMix";
    qDeleteAll(_controls);
    qDeleteAll(_products);
}

/**
 * Clears the GUIProfile cache. You must only call this
 * before termination of the application, as GUIProfile instances are used in other classes, especially the views.
 * There is no need to call this in non-GUI applications like kmixd and kmixctrl.
 */
void GUIProfile::clearCache()
{
	qDeleteAll(s_profiles);
	s_profiles.clear();
}


/**
 * Build a profile name. Suitable to use as primary key and to build filenames.
 * @arg mixer         The mixer
 * @arg profileName   The profile name (e.g. "capture", "playback", "my-cool-profile", or "any"
 * @return            The profile name
 */
static QString buildProfileName(const Mixer *mixer, const QString &profileName, bool ignoreCard)
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
static QString buildReadableProfileName(const Mixer *mixer, const QString &profileName)
{
    QString fname;
    fname += mixer->getBaseName();
    if ( mixer->getCardInstance() > 1 ) {
        fname += " %1";
        fname = fname.arg(mixer->getCardInstance());
    }
    if ( profileName != "default" ) {
        fname += ' ' + profileName;
    }

    qCDebug(KMIX_LOG) << fname;
    return fname;
}

/**
 * Returns the GUIProfile for the given ID (= "fullyQualifiedName").
 * If not found 0 is returned. There is no try to load it.
 * 
 * @returns The loaded GUIProfile for the given ID
 */
GUIProfile *GUIProfile::find(const QString &id)
{
	// Return found value or default-constructed one (nullptr).
	// Does not insert into map.  Now thread-safe.
	return (s_profiles.value(id));
}


static QString createNormalizedFilename(const QString &profileId)
{
	QString profileIdNormalized(profileId);
	profileIdNormalized.replace(':', '.');

	return profileIdNormalized + ".xml";
}


/**
 * Loads a GUI Profile from disk (XML profile file).
 * It tries to load the Soundcard specific file first (a).
 * If it doesn't exist, it will load the default profile corresponding to the soundcard driver (b).
 */
static GUIProfile *loadProfileFromXMLfiles(const Mixer *mixer, const QString &profileName)
{
    GUIProfile* guiprof = nullptr;
    QString fileName = s_profileDir + '/' + createNormalizedFilename(profileName);
    QString fileNameFQ = QStandardPaths::locate(QStandardPaths::AppDataLocation, fileName );

    if ( ! fileNameFQ.isEmpty() ) {
        guiprof = new GUIProfile();
        if ( guiprof->readProfile(fileNameFQ) && ( guiprof->match(mixer) > 0) ) {
            // loaded
        }
        else {
            delete guiprof; // not good (e.g. Parsing error => drop this profile silently)
            guiprof = nullptr;
        }
    }
    else {
        qCDebug(KMIX_LOG) << "Ignore file " <<fileName<< " (does not exist)";
    }
    return guiprof;
}


/*
 * Add the profile to the internal list of profiles (Profile caching).
 */
static void addProfile(GUIProfile *guiprof)
{
	// Possible TODO: Delete old mapped GUIProfile, if it exists. Otherwise we might leak one GUIProfile instance
	//                per unplug/plug sequence. Its quite likely possible that currently no Backend leads to a
	//                leak: This is because they either don't hotplug cards (PulseAudio, MPRIS2), or they ship
	//                a XML gui profile (so the Cached version is retrieved, and addProfile() is not called).

    s_profiles[guiprof->getId()] = guiprof;
    qCDebug(KMIX_LOG) << "I have added" << guiprof->getId() << "; Number of profiles is now " <<  s_profiles.size() ;
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
GUIProfile *GUIProfile::find(const Mixer *mixer, const QString &profileName, bool profileNameIsFullyQualified, bool ignoreCardName)
{
    GUIProfile *guiprof = nullptr;

    if (mixer==nullptr || profileName.isEmpty()) return (nullptr);

//    if ( mixer->isDynamic() ) {
//        qCDebug(KMIX_LOG) << "GUIProfile::find() Not loading GUIProfile for Dynamic Mixer (e.g. PulseAudio)";
//        return 0;
//    }

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
        if ( guiprof!=nullptr) {
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
    
    return (guiprof);
}


/**
 * Returns a fallback GUIProfile. You can call this if the backends ships no profile files.
 * The returned GUIProfile is also added to the static Map of all GUIProfile instances.
 */
GUIProfile* GUIProfile::fallbackProfile(const Mixer *mixer)
{
	// -1- Get name
    QString fullQualifiedProfileName = buildProfileName(mixer, QString("default"), false);

    GUIProfile *fallback = new GUIProfile();

    // -2- Fill details
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

    /* -3- Add the profile to the static list
     *     Hint: This looks like a memory leak, as we never remove profiles from memory while KMix runs.
     *           Especially with application streams it looks suspicious. But please be aware that this method is only
     *           called for soundcard hotplugs, and not on stream hotplugs. At least it is supposed to be like that.
     *
     *           Please also see the docs at addProfile(), they also address the possible memory leakage.
     */
    addProfile(fallback);

    return fallback;
}


/**
 * Fill the profile with the data from the given XML profile file.
 * @par  ref_fileName: Full qualified filename (with path).
 * @return bool True, if the profile was successfully created. False if not (e.g. parsing error).
 */
bool GUIProfile::readProfile(const QString &ref_fileName)
{
    QXmlSimpleReader xmlReader;
    qCDebug(KMIX_LOG) << "Read profile" << ref_fileName;
    QFile xmlFile(ref_fileName);
    QXmlInputSource source(&xmlFile);
    GUIProfileParser gpp(this);
    xmlReader.setContentHandler(&gpp);
    bool ok = xmlReader.parse(source);

    //std::cout << "Raw Profile: " << *this;
    if ( ok ) {
        ok = finalizeProfile();
    } // Read OK
    else {
        // !! this error message about faulty profiles should probably be surrounded with i18n()
        qCCritical(KMIX_LOG) << "ERROR: The profile" << ref_fileName<< "contains errors, and cannot be used";
    }

    return ok;
}


bool GUIProfile::writeProfile()
{
   QString profileId = getId();
   QDir profileDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + '/' + s_profileDir);
   QString fileName = createNormalizedFilename(profileId);
   QString fileNameFQ = profileDir.filePath(fileName);

   if (!profileDir.exists())
       profileDir.mkpath(".");

   qCDebug(KMIX_LOG) << "Write profile" << fileNameFQ;
   QSaveFile f(fileNameFQ);
   if (!f.open(QIODevice::WriteOnly|QFile::Truncate))
   {
      qCWarning(KMIX_LOG) << "Cannot save profile to" << fileNameFQ;
      return (false);
   }

   QXmlStreamWriter writer(&f);
   writer.setAutoFormatting(true);

   // <?xml version="1.0" encoding="utf-8"?>
   writer.writeStartDocument();

   //  <soundcard
   writer.writeStartElement("soundcard");
   //    driver=	_soundcardDriver
   writer.writeAttribute("driver", _soundcardDriver);
   //    version=	(_driverVersionMin << ":" << _driverVersionMax)
   writer.writeAttribute("version", QString("%1:%2").arg(_driverVersionMin).arg(_driverVersionMax));
   //    name=		guiprof._soundcardName
   writer.writeAttribute("name", _soundcardName);
   //    type=		guiprof._soundcardType
   writer.writeAttribute("type", _soundcardType);
   //    generation=	guiprof._generation
   writer.writeAttribute("generation", QString::number(_generation));

   //  <profile
   writer.writeStartElement("profile");
   //    id=		guiprof._id
   writer.writeAttribute("id", _id);
   //    name=		guiprof._name
   writer.writeAttribute("name", _name);
   //  />
   writer.writeEndElement();

   foreach (const ProfProduct *prd, qAsConst(_products))
   {
      //  <product
      writer.writeStartElement("product");
      //    vendor=		prd->vendor
      writer.writeAttribute("vendor", prd->vendor);
      //    name=		prd->productName
      writer.writeAttribute("name", prd->productName);
      //    release=		prd->productRelease
      if (!prd->productRelease.isNull()) writer.writeAttribute("release", prd->productRelease);
      //	 comment=	prd->comment
      if (!prd->comment.isNull()) writer.writeAttribute("comment", prd->comment);
      //  />
      writer.writeEndElement();
   }							// for all products

   foreach (const ProfControl *profControl, qAsConst(getControls()))
   {
      //  <control
      writer.writeStartElement("control");
      //    id=			profControl->id()
      writer.writeAttribute("id", profControl->id());
      //    name=		profControl->name()
      const QString name = profControl->name();
      if (!name.isNull() && name!=profControl->id()) writer.writeAttribute("name", name);
      //    subcontrols=	profControl->renderSubcontrols()
      writer.writeAttribute("subcontrols", profControl->renderSubcontrols());
      //    show=		visibilityToString(profControl->getVisibility())
      writer.writeAttribute("show", visibilityToString(profControl->getVisibility()));
      //    mandatory=		"true"
      if (profControl->isMandatory()) writer.writeAttribute("mandatory", "true");
      //    split=		"true"
      if (profControl->isSplit())  writer.writeAttribute("split", "true");
      //  />
      writer.writeEndElement();
   }							// for all controls

   //  </soundcard>
   writer.writeEndElement();

   writer.writeEndDocument();
   if (writer.hasError())
   {
	   qCWarning(KMIX_LOG) << "XML writing failed to" << fileNameFQ;
	   f.cancelWriting();
	   return (false);
   }

   f.commit();
   _dirty = false;
   return (true);
}


/** This is now empty. It can be removed */
bool GUIProfile::finalizeProfile() const
{
    return (true);
}


// -------------------------------------------------------------------------------------
void GUIProfile::setControls(ControlSet& newControlSet)
{
    qDeleteAll(_controls);
    _controls = newControlSet;
}

// -------------------------------------------------------------------------------------


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
unsigned long GUIProfile::match(const Mixer *mixer) const
{
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


ProfControl::ProfControl(const QString &id, const QString &subcontrols)
	: _id(id),
	  _visibility(GuiVisibility::Simple),
	  _mandatory(false),
	  _split(false)
{
    setSubcontrols(subcontrols);
}

ProfControl::ProfControl(const ProfControl &profControl)
	: _mandatory(false),
	  _split(false)
{
    _id = profControl._id;
    _name = profControl._name;
    _visibility = profControl._visibility;

    _useSubcontrolPlayback = profControl._useSubcontrolPlayback;
    _useSubcontrolCapture = profControl._useSubcontrolCapture;
    _useSubcontrolPlaybackSwitch = profControl._useSubcontrolPlaybackSwitch;
    _useSubcontrolCaptureSwitch = profControl._useSubcontrolCaptureSwitch;
    _useSubcontrolEnum = profControl._useSubcontrolEnum;
    _subcontrols = profControl._subcontrols;

    _backgroundColor = profControl._backgroundColor;
    _switchtype = profControl._switchtype;
    _mandatory = profControl._mandatory;
    _split = profControl._split;
}


bool ProfControl::satisfiesVisibility(GuiVisibility vis) const
{
	GuiVisibility me = getVisibility();
	if (me==GuiVisibility::Never || vis==GuiVisibility::Never) return (false);
	if (me==GuiVisibility::Custom || vis==GuiVisibility::Custom) return (false);
	if (vis==GuiVisibility::Default) return (true);
	return (static_cast<int>(me)<=static_cast<int>(vis));
}


/**
 * An overridden method that either sets
 * GuiVisibility::Simple or GuiVisibility::Never.
 */
void ProfControl::setVisible(bool visible)
{
	setVisibility(visible ? GuiVisibility::Simple : GuiVisibility::Never);
}

void ProfControl::setVisibility(GuiVisibility vis)
{
	_visibility = vis;
}

void ProfControl::setVisibility(const QString &visString)
{
	setVisibility(visibilityFromString(visString));
}

void ProfControl::setSubcontrols(const QString &sctls)
{
    _subcontrols = sctls;

  _useSubcontrolPlayback = false;
  _useSubcontrolCapture = false;
  _useSubcontrolPlaybackSwitch = false;
  _useSubcontrolCaptureSwitch = false;
  _useSubcontrolEnum = false;

  QStringList qsl = sctls.split( ',',  QString::SkipEmptyParts, Qt::CaseInsensitive);
  QStringListIterator qslIt(qsl);
  while (qslIt.hasNext()) {
    QString sctl = qslIt.next();
       //qCDebug(KMIX_LOG) << "setSubcontrols found: " << sctl.toLocal8Bit().constData();
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
       else qCWarning(KMIX_LOG) << "Ignoring unknown subcontrol type '" << sctl << "' in profile";
  }
}

QString ProfControl::renderSubcontrols() const
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
	_scope = GUIProfileParser::NONE;  // no scope yet
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
				qCWarning(KMIX_LOG) << "Ignoring unsupported element" << qName;
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
				qCWarning(KMIX_LOG) << "Ignoring unsupported element" << qName;
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

		_guiProfile->addProduct(prd);
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

	profControl->setName(name);
	profControl->setVisibility(show.isNull() ? "all" : show);
	profControl->setBackgroundColor( background );
	profControl->setSwitchtype(switchtype);
	profControl->setMandatory(isMandatory);
        if (split=="true") profControl->setSplit(true);

	_guiProfile->addControl(profControl);
  } // id != null
}

void GUIProfileParser::printAttributes(const QXmlAttributes& attributes)
{
	if (attributes.length() > 0 ) {
		for ( int i = 0 ; i < attributes.length(); i++ ) {
			qCDebug(KMIX_LOG) << i << attributes.qName(i) << "="<< attributes.value(i);
		}
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
