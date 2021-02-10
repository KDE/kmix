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

#ifndef GUIPROFILE__H
#define GUIPROFILE__H

#include <qstring.h>
#include <qlist.h>

#include <set>


class Mixer;
class QXmlStreamAttributes;


struct ProfProduct
{
    QString vendor;
    QString productName;
    // In case the vendor ships different products under the same productName
    QString productRelease;
    QString comment;
};


/**
 * GuiVisibility can be used in different contexts. One is, to define in the XML GUI Profile, which control to show, e.g. show
 * "MIC Boost" in EXTENDED mode. The other is for representing the GUI complexity (e.g. for letting the user select a preset like "SIMPLE".
 */
enum class GuiVisibility
{
    Simple,
    Extended,
    Full,
    Custom,
    Never,
    Default
};


class ProfControl
{
public:
    ProfControl(const QString &id, const QString &subcontrols);
    ProfControl(const ProfControl &ctl); // copy constructor
    ~ProfControl() = default;

    // ID as returned by the Mixer Backend, e.g. "Master:0"
    QString id() const				{ return (_id); }
    void setId(const QString &id)		{ _id = id; }

    // Visible name for the User (if null, 'id' will be used).
    // And in the future a default lookup table will be consulted.
    // Because the name is visible, some kind of i18n() should be used.
    QString name() const			{ return (_name); }
    void setName(const QString &name)		{ _name = name; }

    void setSubcontrols(const QString &sctls);
    bool useSubcontrolPlayback() const		{ return (_useSubcontrolPlayback); }
    bool useSubcontrolCapture() const		{ return (_useSubcontrolCapture); }
    bool useSubcontrolPlaybackSwitch() const	{ return (_useSubcontrolPlaybackSwitch); }
    bool useSubcontrolCaptureSwitch() const	{ return (_useSubcontrolCaptureSwitch); }
    bool useSubcontrolEnum() const		{ return (_useSubcontrolEnum); }
    QString renderSubcontrols() const;

    QString getBackgroundColor() const		{ return (_backgroundColor); }
    void setBackgroundColor(const QString &col)	{ _backgroundColor = col; }
    QString getSwitchtype() const		{ return (_switchtype); }
    void setSwitchtype(const QString &swtype)	{ _switchtype = swtype; }

    void setVisible(bool visible);
    void setVisibility(GuiVisibility vis);
    void setVisibility(const QString &visString);
    GuiVisibility getVisibility() const		{ return (_visibility); }

    bool isMandatory() const			{ return (_mandatory); }
    void setMandatory(bool mandatory)		{ _mandatory = mandatory; }

    void setSplit (bool split)			{ _split = split; }
    bool isSplit() const			{ return (_split); }

    /**
      * Returns whether this ProfControl's GuiVisibility satisfies the other GuiVisibility.
      * 'Never' can never be satisfied - if this or the other is 'Never', the result is false.
      * 'Custom' is always satisfied - if this or the other is 'Custom', the result is true.
      * 'Default' for the other is always satisfied.
      * The other 3 enum values are completely ordered as 'Simple' < 'Extended' < 'Full'.
      * <p>
      * For example, 'Simple' satisfies 'Full', as the simple GUI is part of the full GUI.
      */
    bool satisfiesVisibility(GuiVisibility vis) const;

private:
    // The following are the deserialized values of _subcontrols
    bool _useSubcontrolPlayback;
    bool _useSubcontrolCapture;
    bool _useSubcontrolPlaybackSwitch;
    bool _useSubcontrolCaptureSwitch;
    bool _useSubcontrolEnum;

    QString _id;
    QString _name;

    // For applying custom colors
    QString _backgroundColor;
    // For defining the switch type when it is not a standard playback or capture switch
    QString _switchtype;

    // show or hide (contains the GUI type: simple, extended, all)
    GuiVisibility _visibility;

    bool _mandatory; // A mandatory control must be included in all GUIProfile copies
    bool _split; // true if this widget is to show two sliders

    // List of controls, e.g: "rec:1-2,recswitch"
    // THIS IS RAW DATA AS LOADED FROM THE PROFILE. DO NOT USE IT, except for debugging.
    QString _subcontrols;
};


struct ProductComparator
{
    bool operator()(const ProfProduct*, const ProfProduct*) const;
};


class GUIProfile
{
public:
    typedef std::set<ProfProduct *, ProductComparator> ProductSet;
    typedef QList<ProfControl *> ControlSet;

public:
    GUIProfile();
    ~GUIProfile();

    bool readProfile(const QString &fileName);
    bool writeProfile();

    bool isDirty() const			{ return (_dirty); }
    void setDirty()				{ _dirty = true; }
    
    void setId(const QString &id)		{ _id = id; }
    QString getId() const			{ return (_id); }
    QString getMixerId() const			{ return (_mixerId); }
    
    unsigned long match(const Mixer *mixer) const;

    static void clearCache();
    static GUIProfile *find(const QString &id);
    static GUIProfile *find(const Mixer *mixer, const QString &profileName, bool profileNameIsFullyQualified, bool ignoreCardName);
    static GUIProfile *fallbackProfile(const Mixer *mixer);

    // --- Getters and setters ----------------------------------------------------------------------
    const ControlSet &getControls() const	{ return (_controls); }

    void setControls(ControlSet &newControlSet);
    void addControl(ProfControl *ctrl)		{ _controls.push_back(ctrl); }
    void addProduct(ProfProduct *prod)		{ _products.insert(prod); }

    QString getName() const			{ return (_name); }
    void setName(const QString &name)		{ _name = name; }

    // --- The values from the <soundcard> tag: No getters and setters for them (yet) -----------------------------
    QString _soundcardDriver;
    // The driver version: 1000*1000*MAJOR + 1000*MINOR + PATCHLEVEL
    unsigned long _driverVersionMin;
    unsigned long _driverVersionMax;
    QString _soundcardName;
    QString _soundcardType;
    unsigned long _generation;

private:
    ControlSet _controls;
    ProductSet _products;
    
    QString _id;
    QString _name;
    QString _mixerId;
    bool _dirty;
};


class GUIProfileParser
{
public:
    explicit GUIProfileParser(GUIProfile *ref_gp);

    void addControl(const QXmlStreamAttributes &attributes);
    void addProduct(const QXmlStreamAttributes &attributes);
    void addSoundcard(const QXmlStreamAttributes &attributes);
    void addProfileInfo(const QXmlStreamAttributes &attributes);

private:
    GUIProfile *_guiProfile;
};

#endif // GUIPROFILE__H
