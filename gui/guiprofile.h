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

#ifndef _GUIPROFILE_H_
#define _GUIPROFILE_H_

class Mixer;

#include <qxml.h>
#include <QColor>
#include <QTextStream>
#include <QString>

#include <string>
#include <map>
#include <set>
#include <vector>
#include <ostream>

#include <KDebug>

struct SortedStringComparator
{
    bool operator()(const std::string&, const std::string&) const;
};


struct ProfProduct
{
    QString vendor;
    QString productName;
    // In case the vendor ships different products under the same productName
    QString productRelease;
    QString comment;
};

class ProfControlPrivate
{
public:
    // List of controls, e.g: "rec:1-2,recswitch"
    // THIS IS RAW DATA AS LOADED FROM THE PROFILE. DO NOT USE IT, except for debugging.
    QString subcontrols;

};

class ProfControl
{
public:
    ProfControl(QString& id, QString& subcontrols);
    ProfControl(const ProfControl &ctl); // copy constructor
    ~ProfControl();
    // ID as returned by the Mixer Backend, e.g. Master:0
    QString id;

    void setSubcontrols(QString sctls);
    bool useSubcontrolPlayback() {return _useSubcontrolPlayback;};
    bool useSubcontrolCapture() {return _useSubcontrolCapture;};
    bool useSubcontrolPlaybackSwitch() {return _useSubcontrolPlaybackSwitch;};
    bool useSubcontrolCaptureSwitch() {return _useSubcontrolCaptureSwitch;};
    bool useSubcontrolEnum() {return _useSubcontrolEnum;};
    QString renderSubcontrols();

    QString getBackgroundColor() const    {        return backgroundColor;    }
    void setBackgroundColor(QString& backgroundColor)    {        this->backgroundColor = backgroundColor;    }
    QString getSwitchtype() const    {        return switchtype;    }
    void setSwitchtype(QString switchtype)    {        this->switchtype = switchtype;    }

    // Visible name for the User ( if name.isNull(), id will be used - And in the future a default lookup table will be consulted ).
    // Because the name is visible, some kind of i18n() should be used.
    QString name;
    // show or hide (contains the GUI type: simple, extended, all)
    QString show;

    bool isMandatory() const
    {
        return _mandatory;
    }

    void setMandatory(bool _mandatory)
    {
        this->_mandatory = _mandatory;
    }
    void setSplit ( bool split ) {
      _split = split;
    }
    bool isSplit() const { 
      return _split;
    }

private:
    // The following are the deserialized values of _subcontrols
    bool _useSubcontrolPlayback;
    bool _useSubcontrolCapture;
    bool _useSubcontrolPlaybackSwitch;
    bool _useSubcontrolCaptureSwitch;
    bool _useSubcontrolEnum;

    // For applying custom colors
    QString backgroundColor;
    // For defining the switch type when it is not a standard palyback or capture switch
    QString switchtype;

    bool _mandatory; // A mandatory control must be included in all GUIProfile copies

    ProfControlPrivate *d;
    bool _split; // true if this widget is to show two sliders
};


struct ProductComparator
{
    bool operator()(const ProfProduct*, const ProfProduct*) const;
};

class GUIProfile
{
    public:
    GUIProfile();
    virtual ~GUIProfile();

    bool readProfile(const QString& ref_fileNamestring);
    bool finalizeProfile() const;
    bool writeProfile();
    
    bool isDirty() const;
    void setDirty();
    
    void setId(const QString& id);
    QString getId() const;
    QString getMixerId() const { return _mixerId; }
    
    static QMap<QString, GUIProfile*>& getProfiles() { return s_profiles; }
    
    unsigned long match(Mixer* mixer);
    friend std::ostream& operator<<(std::ostream& os, const GUIProfile& vol);
    friend QTextStream& operator<<(QTextStream &outStream, const GUIProfile& guiprof);

    typedef std::set<ProfProduct*, ProductComparator> ProductSet;


    ProductSet _products;

    static GUIProfile* find(Mixer* mixer, QString profileName, bool profileNameIsFullyQualified, bool ignoreCardName);
    static GUIProfile* find(QString id);
    static GUIProfile* selectProfileFromXMLfiles(Mixer*, QString preferredProfile);
    static GUIProfile* fallbackProfile(Mixer*);

    typedef QList<ProfControl*> ControlSet;

    const ControlSet& getControls() const
    {
        return _controls;
    }
    ControlSet& getControls()
    {
        return _controls;
    }
    void setControls(ControlSet& newControlSet);

    QString getName() const    {        return _name;    }
    void setName(QString _name)    {        this->_name = _name;    }

    // The values from the <soundcard> tag
    QString _soundcardDriver;
    // The driver version: 1000*1000*MAJOR + 1000*MINOR + PATCHLEVEL
    unsigned long _driverVersionMin;
    unsigned long _driverVersionMax;
    QString _soundcardName;
    QString _soundcardType;
    unsigned long _generation;
private:
    ControlSet _controls;
    
    // Loading
    static QString buildProfileName(Mixer* mixer, QString profileName, bool ignoreCard);
    static QString buildReadableProfileName(Mixer* mixer, QString profileName);

    static GUIProfile* loadProfileFromXMLfiles(Mixer* mixer, QString profileName);
    static void addProfile(GUIProfile* guiprof);
    static QMap<QString, GUIProfile*> s_profiles;
    
    QString _id;
    QString _name;
    QString _mixerId;
    bool _dirty;
};

std::ostream& operator<<(std::ostream& os, const GUIProfile& vol);
QTextStream& operator<<(QTextStream &outStream, const GUIProfile& guiprof);

class GUIProfileParser : public QXmlDefaultHandler
{
public:
    GUIProfileParser(GUIProfile* ref_gp);
    // Enumeration for the scope
    enum ProfileScope { NONE, SOUNDCARD };
    
    bool startDocument();
    bool startElement( const QString&, const QString&, const QString& , const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );
    
private:
    void addControl(const QXmlAttributes& attributes);
    void addProduct(const QXmlAttributes& attributes);
    void addSoundcard(const QXmlAttributes& attributes);
    void addProfileInfo(const QXmlAttributes& attributes);
    void printAttributes(const QXmlAttributes& attributes);
    void splitPair(const QString& pairString, std::pair<QString,QString>& result, char delim);

    ProfileScope _scope;
    GUIProfile* _guiProfile;
};

#endif //_GUIPROFILE_H_
