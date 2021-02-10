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
#include <QXmlStreamReader>
#include <QStandardPaths>
// KMix
#include "core/mixer.h"


#undef DEBUG_XMLREADER


static QMap<QString, GUIProfile *> s_profiles;
static const QString s_profileDir("profiles");


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
 * @par  fileName: Full qualified filename (with path).
 * @return bool True, if the profile was successfully created. False if not (e.g. parsing error).
 */
bool GUIProfile::readProfile(const QString &fileName)
{
    qCDebug(KMIX_LOG) << "reading" << fileName;

    QFile xmlFile(fileName);
    bool ok = xmlFile.open(QIODevice::ReadOnly);
    if (ok)
    {
        GUIProfileParser gpp(this);

        QXmlStreamReader reader(&xmlFile);
        while (!reader.atEnd())
        {
            bool startOk = reader.readNextStartElement();
            if (!startOk)
            {
#ifdef DEBUG_XMLREADER
                qCDebug(KMIX_LOG) << "  no more start elements";
#endif
                break;
            }

            const QString &name = reader.name().toString().toLower();
            const QXmlStreamAttributes attrs = reader.attributes();
#ifdef DEBUG_XMLREADER
            qCDebug(KMIX_LOG) << "  element" << name << "has" << attrs.count() << "attributes:";
            for (const QXmlStreamAttribute &attr : qAsConst(attrs))
            {
                qCDebug(KMIX_LOG) << "    " << attr.name() << "=" << attr.value();
            }
#endif
            if (name=="soundcard")
            {
                gpp.addSoundcard(attrs);
                continue;				// then read contained elements
            }
            else if (name=="control") gpp.addControl(attrs);
            else if (name=="product") gpp.addProduct(attrs);
            else if (name=="profile") gpp.addProfileInfo(attrs);
            else qCDebug(KMIX_LOG) << "Unknown XML tag" << name << "at line" << reader.lineNumber();

            reader.skipCurrentElement();
        }

        if (reader.hasError())
        {
            qCWarning(KMIX_LOG) << "XML parse error at line" << reader.lineNumber() << "-" << reader.errorString();
            ok = false;
        }
    }

    return (ok);
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

   for (const ProfProduct *prd : qAsConst(_products))
   {
      //  <product
      writer.writeStartElement("product");
      //    vendor=		prd->vendor
      writer.writeAttribute("vendor", prd->vendor);
      //    name=		prd->productName
      writer.writeAttribute("name", prd->productName);
      //    release=		prd->productRelease
      if (!prd->productRelease.isEmpty()) writer.writeAttribute("release", prd->productRelease);
      //	 comment=	prd->comment
      if (!prd->comment.isEmpty()) writer.writeAttribute("comment", prd->comment);
      //  />
      writer.writeEndElement();
   }							// for all products

   for (const ProfControl *profControl : qAsConst(getControls()))
   {
      //  <control
      writer.writeStartElement("control");
      //    id=			profControl->id()
      writer.writeAttribute("id", profControl->id());
      //    name=		profControl->name()
      const QString name = profControl->name();
      if (!name.isEmpty() && name!=profControl->id()) writer.writeAttribute("name", name);
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

#if QT_VERSION>=QT_VERSION_CHECK(5, 14, 0)
  QStringList qsl = sctls.split( ',',  Qt::SkipEmptyParts, Qt::CaseInsensitive);
#else
  QStringList qsl = sctls.split( ',',  QString::SkipEmptyParts, Qt::CaseInsensitive);
#endif

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

GUIProfileParser::GUIProfileParser(GUIProfile *ref_gp)
{
    _guiProfile = ref_gp;
}


void GUIProfileParser::addSoundcard(const QXmlStreamAttributes &attributes)
{
    const QString driver     = attributes.value("driver").toString();
    const QString version    = attributes.value("version").toString();
    const QString name	     = attributes.value("name").toString();
    const QString type	     = attributes.value("type").toString();
    const QString generation = attributes.value("generation").toString();

    // Adding a card makes only sense if we have at least
    // the driver and product name.
    if (driver.isEmpty() || name.isEmpty() ) return;

    _guiProfile->_soundcardDriver = driver;
    _guiProfile->_soundcardName = name;
    _guiProfile->_soundcardType = type;

    if (version.isEmpty())
    {
        _guiProfile->_driverVersionMin = 0;
        _guiProfile->_driverVersionMax = 0;
    }
    else
    {
        const QStringList versionMinMax = version.split(':', Qt::KeepEmptyParts);
        _guiProfile->_driverVersionMin = versionMinMax.value(0).toULong();
        _guiProfile->_driverVersionMax = versionMinMax.value(1).toULong();
    }

    _guiProfile->_generation = generation.toUInt();
}


void GUIProfileParser::addProfileInfo(const QXmlStreamAttributes& attributes)
{
    const QString name = attributes.value("name").toString();
    const QString id   = attributes.value("id").toString();

    _guiProfile->setId(id);
    _guiProfile->setName(name);
}


void GUIProfileParser::addProduct(const QXmlStreamAttributes& attributes)
{
    const QString vendor  = attributes.value("vendor").toString();
    const QString name    = attributes.value("name").toString();
    const QString release = attributes.value("release").toString();
    const QString comment = attributes.value("comment").toString();

    // Adding a product makes only sense if we have at least
    // the vendor and product name.
    if (vendor.isEmpty() || name.isEmpty()) return;

    ProfProduct *prd = new ProfProduct();
    prd->vendor = vendor;
    prd->productName = name;
    prd->productRelease = release;
    prd->comment = comment;

    _guiProfile->addProduct(prd);
}


void GUIProfileParser::addControl(const QXmlStreamAttributes &attributes)
{
    const QString id          = attributes.value("id").toString();
    const QString subcontrols = attributes.value("subcontrols").toString();
    const QString name        = attributes.value("name").toString();
    const QString show        = attributes.value("show").toString();
    const QString background  = attributes.value("background").toString();
    const QString switchtype  = attributes.value("switchtype").toString();
    const QString mandatory   = attributes.value("mandatory").toString();
    const QString split       = attributes.value("split").toString();

    // We need at least an "id".  We can set defaults for the rest, if undefined.
    if (id.isEmpty()) return;

    // ignore whether 'name' is null, will be checked by all users.
    bool isMandatory = (mandatory=="true");
    // ignore whether 'background' is null, will be checked by all users.
    // ignore whether 'switchtype' is null, will be checked by all users.

    // For compatibility reasons, we interpret an empty string as match-all (aka "*")
    ProfControl *profControl = new ProfControl(id, (subcontrols.isEmpty() ? "*" : subcontrols));

    profControl->setName(name);
    profControl->setVisibility(show.isEmpty() ? "all" : show);
    profControl->setBackgroundColor(background);
    profControl->setSwitchtype(switchtype);
    profControl->setMandatory(isMandatory);
    if (split=="true") profControl->setSplit(true);

    _guiProfile->addControl(profControl);
}
