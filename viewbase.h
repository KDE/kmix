#ifndef ViewBase_h
#define ViewBase_h

// QT
#include "qwidget.h"

// KDE
class KActionCollection;
class KPopupMenu;
class MixSet;
class Mixer;
class MixDevice;

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

    ViewBase(QWidget* parent, const char* name, const QString & caption, Mixer* mixer, WFlags=0, ViewFlags vflags=0);
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
    virtual KPopupMenu* getPopup();
    virtual void popupReset();
    virtual void showContextMenu();

    Mixer* getMixer();

    /**
     * Contains the widgets for the _mixSet. There is a 1:1 relationship, which means:
     * _mdws[i] is the Widget for the MixDevice _mixSet[i].
     * Hint: The new ViewSurround class shows that a 1:1 relationship does not work in a general scenario.
     *       I actually DID expect this. The solution is unclear yet, probably there will be a virtual mapper method.
     */
    QPtrList<QWidget> _mdws; // this obsoletes the former Channel class
    
    QString caption() const { return _caption; }

protected:
    void init();

    Mixer *_mixer;
    MixSet *_mixSet;
    KPopupMenu *_popMenu;
    KActionCollection* _actions;
    ViewFlags _vflags;

public slots:
   virtual void refreshVolumeLevels();
   virtual void configureView(); 
   void toggleMenuBarSlot();

protected slots:
   void mousePressEvent( QMouseEvent *e );

signals:
   void toggleMenuBar();
   
private:
   QString _caption;
};

#endif

