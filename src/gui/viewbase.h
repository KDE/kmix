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
#include <QSet>
#include <QWidget>
#include <QList>

// KDE
#include <KActionCollection>
class KMenu;

#include "core/mixset.h"

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

friend class KMixToolBox;  // the toolbox is everybodys friend :-)

public:

   typedef uint ViewFlags;
   enum ViewFlagsEnum {
      // Regular flags
      HasMenuBar     = 0x0001,
      MenuBarVisible = 0x0002,
      Horizontal     = 0x0004,
      Vertical       = 0x0008
   };

    ViewBase(QWidget* parent, const char* id, Mixer* mixer, Qt::WFlags=0, ViewFlags vflags=0, GUIProfile *guiprof=0, KActionCollection* actionCollection = 0);
    virtual ~ViewBase();

    QString id() const;

    // This method is called by ViewBase at the end of createDeviceWidgets(). The default
    // implementation does nothing. Subclasses can override this method for doing final
    // touches. This is very much like polish(), but called at an exactly well-known time.
    // Also I do not want Views to interfere with polish()
    virtual void constructionFinished() = 0;

    // This method is called after a configuration update (show/hide controls, split/unsplit).
    // More complicated changes (e.g. order of controls) need a GUI rebuild - please use 
    // rebuildFromProfile() then.
    // The default implementation does nothing.
    virtual void configurationUpdate();

    // This method is called after a configuration update (in other words: after the user
    // has clicked "OK" on the "show/hide" configuration dialog. The default implementation
    // does nothing.
    virtual void rebuildFromProfile();


    void load(KConfig *config);
    void save(KConfig *config);

    /**
     * Creates the widgets for all supported devices. The default implementation loops
     * over the supported MixDevice's and calls add() for each of it.
     */
    virtual void createDeviceWidgets();

    void setMixSet();
    int visibleControls();
    
    bool isDynamic() const;

    /**
     * Creates a suitable representation for the given MixDevice.
     */
    virtual QWidget* add(shared_ptr<MixDevice>) = 0;

    /**
     * Popup stuff
     */
    virtual KMenu* getPopup();
    virtual void popupReset();
    virtual void showContextMenu();

    virtual bool isValid() const;

   void setIcons(bool on);
   void setLabels(bool on);
   void setTicks(bool on);
   GUIProfile* guiProfile() { return _guiprof; };
   KActionCollection* actionCollection() { return _actions; };

   QSet<Mixer*>& getMixers() { return _mixers; };

    /**
     * Contains the widgets for the _mixSet. There is a 1:1 relationship, which means:
     * _mdws[i] is the Widget for the MixDevice _mixSet[i] - please see ViewBase::createDeviceWidgets().
     * Hint: !! The new ViewSurround class shows that a 1:1 relationship does not work in a general scenario.
     *       I actually DID expect this. The solution is unclear yet, probably there will be a virtual mapper method.
     */
    QList<QWidget *> _mdws;

signals:
    void rebuildGUI();
    //void redrawMixer( const QString& mixer_ID );


protected:
    MixSet _mixSet;
    Mixer *_mixer;
    QSet<Mixer*> _mixers; // this might deprecate _mixer in the future. Currently only in use by ViewDockAreaPopup
    KMenu *_popMenu;
    KActionCollection* _actions; // -<- applciations wide action collection

    ViewFlags _vflags;
    GUIProfile* _guiprof;
    KActionCollection *_localActionColletion;

    virtual void _setMixSet() = 0;

public slots:
   virtual void controlsReconfigured( const QString& mixer_ID );
   virtual void refreshVolumeLevels();
   virtual void configureView(); 
   void toggleMenuBarSlot();

protected slots:
   void mousePressEvent( QMouseEvent *e );

signals:
   void toggleMenuBar();

private:
   QString      m_viewId;
};

#endif

