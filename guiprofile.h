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

class ProfControl
{
public:
        ProfControl(); // default constructor
        ProfControl(const ProfControl &ctl); // copy constructor
	// ID as returned by the Mixer Backend, e.g. Master:0

/*
        bool operator==(QString& other ) const
        {
                return other == id;
        }

        bool operator==(QString other ) const
        {
                return other == id;
        }

        bool operator==(const QString& other ) const
        {
                return other == id;
        }
*/
	QString id;
	
    
    
	void setSubcontrols(QString sctls);
	bool useSubcontrolPlayback() {return _useSubcontrolPlayback;};
	bool useSubcontrolCapture() {return _useSubcontrolCapture;};
	bool useSubcontrolPlaybackSwitch() {return _useSubcontrolPlaybackSwitch;};
	bool useSubcontrolCaptureSwitch() {return _useSubcontrolCaptureSwitch;};
	bool useSubcontrolEnum() {return _useSubcontrolEnum;};
	
	// In case the vendor ships different products under the same productName
	QString tab;
	// Visible name for the User ( if name.isNull(), id will be used - And in the future a default lookup table will be consulted ).
	// Because the name is visible, some kind of i18n() will be used.
	QString name;
   // Pattern (REGEXP) for matching the control names.
   // If you set no pattern, the name will be used instead.
   // If you use a pattern, you normally should not define a name, as it will apply to all matching controls
   QString regexp;
	// show or hide (contains the GUI type: simple, extended, all)
	QString show;
      // For applying custom colors
      QColor backgroundColor;
      // For defining the switch type when it is not a standard palyback or capture switch
      QString switchtype;

  private:
    // List of controls, e.g: "rec:1-2,recswitch"
    // When we start using it, it might be changed into a std::vector in the future.
    // THIS IS RAW DATA AS LOADED FROM THE PROFILE. DO NOT USE IT, except for debugging.
    QString _subcontrols;
    // The follwoing are the deserialized values of _subcontrols
    bool _useSubcontrolPlayback;
    bool _useSubcontrolCapture;
    bool _useSubcontrolPlaybackSwitch;
    bool _useSubcontrolCaptureSwitch;
    bool _useSubcontrolEnum;

};

class ProfTab
{
    public:
        ProfTab();
    // Name of the Tab, in english
    QString name() { return _name; }
    // ID of the Tab
    QString id() { return _id; }
    // Type of the Tab, either "play", "record" or "switches"
    QString type() { return _type; }
 
    void setName(QString name) { _name = name; _id = name; }  // until we explicitely use ID, lets assign ID the same value as NAME
    void setId(QString id) { _id = id; }
    void setType(QString typ) { _type = typ; }
    
    private:
        QString _id;
        QString _name;
        QString _type;
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

	bool readProfile(QString& ref_fileNamestring, QString ref_fileNameWithoutFullPath);
	bool finalizeProfile();
	bool writeProfile(QString& fname);
	unsigned long match(Mixer* mixer);
	friend std::ostream& operator<<(std::ostream& os, const GUIProfile& vol);
	friend QTextStream& operator<<(QTextStream &outStream, const GUIProfile& guiprof);
 
	// key, value, comparator
	//typedef std::map<std::string, std::string, SortedStringComparator> SortedStringMap;
	typedef std::set<ProfProduct*, ProductComparator> ProductSet;
	typedef std::vector<ProfControl*> ControlSet;
	//typedef std::map<std::string, std::string, SortedStringComparator> SortedStringSet;
	//typedef std::map<std::string, std::string> StringMap;
	ControlSet _controls;
	
    QList<ProfTab*>& tabs() { return _tabs; };
    QList<ProfTab*> tabs() const { return _tabs; };
	ProductSet _products;

    static GUIProfile* find(Mixer* mixer, QString preferredProfile);
    static GUIProfile* selectProfileFromXMLfiles(Mixer*, QString preferredProfile);
    static GUIProfile* fallbackProfile(Mixer*);
    
	// The values from the <soundcard> tag
	QString _soundcardDriver;
	// The driver version: 1000*1000*MAJOR + 1000*MINOR + PATCHLEVEL
	unsigned long _driverVersionMin;
	unsigned long _driverVersionMax;
	QString _soundcardName;
	QString _soundcardType;
	unsigned long _generation;
private:
	QString _fileNameWithoutFullPath;
	QList<ProfTab*> _tabs;        // shouldn't be sorted
	static QMap<Mixer*,GUIProfile*> s_fallbackProfiles;
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
		void addTab(const QXmlAttributes& attributes);
		void printAttributes(const QXmlAttributes& attributes);
		void splitPair(const QString& pairString, std::pair<QString,QString>& result, char delim);

		ProfileScope _scope;
		GUIProfile* _guiProfile;
};

#endif //_GUIPROFILE_H_
