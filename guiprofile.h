/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2006-2007 Christian Esken
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
#include <QString>
#include <string>
#include <map>
#include <set>
#include <vector>

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

struct ProfControl
{
	// ID as returned by the Mixer Backend, e.g. Master:0
	QString id;
	// List of controls, e.g: "rec:1-2,recswitch"
	// When we start using it, it might be changed into a std::vector in the future.
	QString subcontrols;
	// In case the vendor ships different products under the same productName
	QString tab;
	// Visible name for the User ( if name.isNull(), id will be used - And in the future a default lookup table will be consulted ).
	// Because the name is visible, some kind of i18n() will be used.
	QString name;
	// show or hide (contains the GUI type: simple, extended, full)
	QString show;
};

struct ProfTab
{
	// Name of the Tab, in english
	QString name;
	// Type of the Tab, either "play", "record" or "switches"
	QString type;
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

	bool readProfile(QString& ref_fileNamestring);
	unsigned long match(Mixer* mixer);
	friend std::ostream& operator<<(std::ostream& os, const GUIProfile& vol);
 
	// key, value, comparator
	//typedef std::map<std::string, std::string, SortedStringComparator> SortedStringMap;
	typedef std::set<ProfProduct*, ProductComparator> ProductSet;
	typedef std::vector<ProfControl*> ControlSet;
	//typedef std::map<std::string, std::string, SortedStringComparator> SortedStringSet;
	typedef std::map<std::string, std::string> StringMap;
	ControlSet _controls;
	std::vector<ProfTab*> _tabs;        // shouldn't be sorted
	ProductSet _products;

	// The values from the <soundcard> tag
	QString _soundcardDriver;
	// The driver version: 1000*1000*MAJOR + 1000*MINOR + PATCHLEVEL
	unsigned long _driverVersionMin;
	unsigned long _driverVersionMax;
	QString _soundcardName;
	QString _soundcardType;
	unsigned long _generation;
};

std::ostream& operator<<(std::ostream& os, const GUIProfile& vol);

class GUIProfileParser : public QXmlDefaultHandler
{
public:
		GUIProfileParser(GUIProfile& ref_gp);
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
		GUIProfile& _guiProfile;
};

#endif //_GUIPROFILE_H_
