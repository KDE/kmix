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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef ViewBase_h
#define ViewBase_h

// QT
#include <QWidget>
#include <qlist.h>

// KDE
class KActionCollection;
class KMenu;
class MixSet;
class Mixer;
class MixDevice;

// KMix
class GUIProfile;

/**
  * The ViewBase is a virtual base class, to be used for subclassing the real Mixer Views.
  */
class ViewBase : public QWidget
{
    Q_OBJECT
public:

    typedef uint ViewFlags;
    enum ViewFlagsEnum {
	// Regular flags
        HasMenuBar     = 0x0001,
        MenuBarVisible = 0x0002,
        Horizontal     = 0x0004,
        Vertical       = 0x0008,
	// Experimental flags
	Experimental_SurroundView = 0x1000,
	Experimental_GridView = 0x2000
    };

    ViewBase(QWidget* parent, const char* id, Mixer* mixer, Qt::WFlags=0, ViewFlags vflags=0, GUIProfile *guiprof=0);
    virtual ~ViewBase();

    // Subclasses must define this method. It is called by the ViewBase() constuctor.
    // The view class must initialize here the _mixSet. This will normally be a subset
    // of the passed mixset.
    // After that the subclass must be prepared for
    // being fed MixDevice's via the add() method.
    virtual void setMixSet(MixSet *mixset);

    // Returns the number of accepted MixDevice's from setMixerSet(). This is
    // normally smaller that mixset->count(), except when the class creates virtual
    // devices
    virtual int count() = 0;

    QString viewId() const;

    // returns an advice about whether this view should be used at all. The returned
    // value is a percentage (0-100). A view without accepted devices would return 0,
    // a "3D sound View" would return 75, if all vital but some optional devices are
    // not available.
    virtual int advice() = 0;

    // This method is called by ViewBase at the end of createDeviceWidgets(). The default
    // implementation does nothing. Subclasses can override this method for doing final
    // touches. This is very much like polish(), but called at an exactly well-known time.
    // Also I do not want Views to interfere with polish()
    virtual void constructionFinished() = 0;

    // This method is called after a configuration update (in other words: after the user
    // has clicked "OK" on the "show/hide" configuration dialog. The default implementation
    // does nothing.
    virtual void configurationUpdate();

    /**
     * Creates the widgets for all supported devices. The default implementation loops
     * over the supported MixDevice's and calls add() for each of it.
     */
    virtual void createDeviceWidgets();

    /**
     * Creates a suitable representation for the given MixDevice.
     * The default implementation creates a label
     */
    virtual QWidget* add(MixDevice *);

    /**
     * Popup stuff
     */
    virtual KMenu* getPopup();
    virtual void popupReset();
    virtual void showContextMenu();

    Mixer* getMixer();

    /**
     * Contains the widgets for the _mixSet. There is a 1:1 relationship, which means:
     * _mdws[i] is the Widget for the MixDevice _mixSet[i].
     * Hint: !! The new ViewSurround class shows that a 1:1 relationship does not work in a general scenario.
     *       I actually DID expect this. The solution is unclear yet, probably there will be a virtual mapper method.
     */
    QList<QWidget *> _mdws; // this obsoletes the former Channel class

protected:
    void init();

    Mixer *_mixer;
    MixSet *_mixSet;
    KMenu *_popMenu;
    KActionCollection* _actions;
    ViewFlags _vflags;
    GUIProfile* _guiprof;

public slots:
   virtual void refreshVolumeLevels();
   virtual void configureView(); 
   void toggleMenuBarSlot();

protected slots:
   void mousePressEvent( QMouseEvent *e );

signals:
   void toggleMenuBar();

private:
   unsigned int _dummyImplPos;
   QString      m_viewId;
};

#endif

