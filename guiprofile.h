//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
 * Software Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef _GUIPROFILE_H_
#define _GUIPROFILE_H_

#include <qxml.h>
#include <qstring.h>
#include <string>
#include <map>
#include <set>

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
	friend std::ostream& operator<<(std::ostream& os, const GUIProfile& vol);
  
	// key, value, comparator
	//typedef std::map<std::string, std::string, SortedStringComparator> SortedStringMap;
	typedef std::set<ProfProduct*, ProductComparator> ProductSet;
	typedef std::set<ProfControl*> ControlSet;
	typedef std::map<std::string, std::string, SortedStringComparator> SortedStringSet;
	typedef std::map<std::string, std::string> StringMap;
	ControlSet _controls;
	StringMap _tabs;        // shouldn't be sorted
	ProductSet _products;
};

std::ostream& operator<<(std::ostream& os, const GUIProfile& vol);

class GUIProfileParser : public QXmlDefaultHandler
{
public:
		GUIProfileParser::GUIProfileParser(GUIProfile& ref_gp);
		// Enumeration for the scope
		enum ProfileScope { NONE, SOUNDCARD, TAB  };
		
		bool startDocument();
		bool startElement( const QString&, const QString&, const QString& , const QXmlAttributes& );
		bool endElement( const QString&, const QString&, const QString& );
		
private:
		void addControl(const QXmlAttributes& attributes);
		void addProduct(const QXmlAttributes& attributes);
		void addSoundcard(const QXmlAttributes& attributes);
		void addTab(const QXmlAttributes& attributes);
		void printAttributes(const QXmlAttributes& attributes);

		ProfileScope _scope;
		GUIProfile& _guiProfile;
};

#endif //_GUIPROFILE_H_
