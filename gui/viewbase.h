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
#include <QList>
#include <QPushButton>

// KDE
#include <KActionCollection>
class KIcon;
class KMenu;

class Mixer;
class MixDevice;

// KMix
#include "core/mixdevice.h"
#include "core/mixset.h"
#include "gui/guiprofile.h"

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
   
   typedef uint GUIComplexity;
   enum
   {
     SIMPLE = 0,
     EXTENDED = 1,
     ALL = 2
   };
   
    ViewBase(QWidget* parent, QString id, Qt::WFlags, ViewFlags vflags, QString guiProfileId, KActionCollection* actionCollection = 0);
    virtual ~ViewBase();

    void addMixer(Mixer *mixer);
    
    QString id() const;

    // This method is called by ViewBase at the end of createDeviceWidgets(). The default
    // implementation does nothing. Subclasses can override this method for doing final
    // touches. This is very much like polish(), but called at an exactly well-known time.
    // Also I do not want Views to interfere with polish()
    virtual void constructionFinished() = 0;

    /**
     * Creates a suitable representation for the given MixDevice.
     */
    virtual QWidget* add(shared_ptr<MixDevice>) = 0;

    // This method is called after a configuration update (show/hide controls, split/unsplit).
    virtual void configurationUpdate(); // TODO remove

    void load(KConfig *config);
    void save(KConfig *config);

    /**
     * Creates the widgets for all supported devices. The default implementation loops
     * over the supported MixDevice's and calls add() for each of it.
     */
    virtual void createDeviceWidgets();

    int visibleControls();
    
    bool isDynamic() const;
    bool pulseaudioPresent() const;

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
   
   GUIProfile* guiProfile() { return GUIProfile::find(_guiProfileId); };
   GUIComplexity getGuiComplexity() { return guiComplexity; };
   ProfControl* findMdw(const QString& mdwId, QString requestedGuiComplexityName);

   
   KActionCollection* actionCollection() { return _actions; };

   QList<Mixer*>& getMixers() { return _mixers; };

    /**
     * Contains the widgets for the _mixSet. There is a 1:1 relationship, which means:
     * _mdws[i] is the Widget for the MixDevice _mixSet[i] - please see ViewBase::createDeviceWidgets().
     * Hint: !! The new ViewSurround class shows that a 1:1 relationship does not work in a general scenario.
     *       I actually DID expect this. The solution is unclear yet, probably there will be a virtual mapper method.
     */
    QList<QWidget *> _mdws;

protected:
    MixSet _mixSet;
    QList<Mixer*> _mixers;
    KMenu *_popMenu;
    KActionCollection* _actions; // -<- application wide action collection

    ViewFlags _vflags;
    const QString _guiProfileId;
    KActionCollection *_localActionColletion;

    KIcon* configureIcon;

    virtual void _setMixSet() = 0;
    void resetMdws();
    void updateGuiOptions();
    QPushButton* createConfigureViewButton();

    GUIComplexity guiComplexity;

public slots:
   virtual void refreshVolumeLevels(); // TODO remove
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

