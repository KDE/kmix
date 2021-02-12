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
#ifndef VIEWBASE_H
#define VIEWBASE_H

// Qt
#include <QList>
#include <QPushButton>
#include <QFlags>

// KDE
#include <KActionCollection>

class QMenu;
class QContextMenuEvent;

class Mixer;
class MixDevice;
class MixDeviceWidget;

// KMix
#include "core/mixdevice.h"
#include "core/mixset.h"
#include "gui/guiprofile.h"

/**
  * The ViewBase is an abstract base class, to be used for subclassing the real Mixer Views.
  */
class ViewBase : public QWidget
{
    Q_OBJECT

public:
    enum ViewFlag
    {
        HasMenuBar     = 0x0001,
        MenuBarVisible = 0x0002
    };
    Q_DECLARE_FLAGS(ViewFlags, ViewFlag)

    ViewBase(QWidget* parent, const QString &id, Qt::WindowFlags f, ViewFlags vflags, const QString &guiProfileId, KActionCollection* actionCollection = nullptr);
    // The GUI profile will not be removed on destruction,
    // as it is pooled and might be applied to a new View.
    virtual ~ViewBase() = default;

    // This method is called after a configuration update (show/hide controls, split/unsplit).
    virtual void configurationUpdate() = 0;

    void load(const KConfig *config);
    void save(KConfig *config) const;

    QMenu *getPopup();

    bool isDynamic() const;
    bool pulseaudioPresent() const;
    int visibleControls() const;

   void setIcons(bool on);
   void setLabels(bool on);
   void setTicks(bool on);
   
    bool isValid() const;
    QString id() const					{ return (m_viewId); }

    GUIProfile* guiProfile() const			{ return (GUIProfile::find(_guiProfileId)); }

    ProfControl *findMdw(const QString &mdwId, GuiVisibility visibility = GuiVisibility::Default) const;

    KActionCollection *actionCollection() const		{ return (_actions); }

    const QList<Mixer *> &getMixers() const		{ return (_mixers); }
    void clearMixers()					{ _mixers.clear(); }
    const MixSet &getMixSet() const			{ return (_mixSet); }
    void addToMixSet(shared_ptr<MixDevice> md)		{ _mixSet.append(md); }

    int mixDeviceCount() const				{ return (_mdws.count()); }
    QWidget *mixDeviceAt(int i) const			{ return (_mdws.at(i)); }

    Qt::Orientation orientation() const			{ return (_orientation); }

private:
    /**
     * Contains the widgets for the _mixSet. There is a 1:1 relationship, which means:
     * _mdws[i] is the Widget for the MixDevice _mixSet[i] - please see ViewBase::createDeviceWidgets().
     * Hint: !! The new ViewSurround class shows that a 1:1 relationship does not work in a general scenario.
     *       I actually DID expect this. The solution is unclear yet, probably there will be a virtual mapper method.
     */
    QList<QWidget *> _mdws;

    QMenu *_popMenu;
    KActionCollection* _actions; // -<- application wide action collection
    KActionCollection *_localActionColletion;

    ViewFlags _vflags;
    Qt::Orientation _orientation;
    GuiVisibility guiLevel;
    const QString _guiProfileId;

   QString m_viewId;

private:
    void setGuiLevel(GuiVisibility guiLevel);
    void updateMediaPlaybackIcons();
    void popupReset();

private:
    MixSet _mixSet;
    QList<Mixer *> _mixers;

protected:
    void resetMdws();
    void updateGuiOptions();
    QPushButton *createConfigureViewButton();
    void addMixer(Mixer *mixer);

    virtual void initLayout() = 0;
    virtual Qt::Orientation orientationSetting() const = 0;

    void adjustControlsLayout();

    /**
     * Creates the widgets for all supported devices. The default implementation loops
     * over the supported MixDevice's and calls add() for each of it.
     */
    void createDeviceWidgets();

    /**
     * Popup stuff
     */
    void contextMenuEvent(QContextMenuEvent *ev) override;
    virtual void showContextMenu();

    // Creates a suitable representation for the given MixDevice.
    virtual QWidget *add(const shared_ptr<MixDevice>) = 0;

    // This method is called by ViewBase at the end of createDeviceWidgets(). The default
    // implementation does nothing. Subclasses can override this method for doing final
    // touches. This is very much like polish(), but called at an exactly well-known time.
    // Also I do not want Views to interfere with polish()
    virtual void constructionFinished() = 0;

public slots:
   virtual void refreshVolumeLevels(); // TODO remove
   virtual void configureView();

signals:
   void toggleMenuBar();

private slots:
   void guiVisibilitySlot(MixDeviceWidget* source, bool enable);
   void toggleMenuBarSlot();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ViewBase::ViewFlags)

#endif
