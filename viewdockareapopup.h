#ifndef ViewDockAreaPopup_h
#define ViewDockAreaPopup_h

#include "viewbase.h"

class QMouseEvent;
class QGridLayout;
class QWidget;
class QPushButton;

class Mixer;
class KMixDockWidget;
class MixDeviceWidget;
class MixDevice;
class QFrame;
class QTime;

class ViewDockAreaPopup : public ViewBase
{
    Q_OBJECT
public:
    ViewDockAreaPopup(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, KMixDockWidget *dockW);
    ~ViewDockAreaPopup();
    MixDevice* dockDevice();

    virtual int count();
    virtual int advice();
    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();
    virtual void refreshVolumeLevels();
    virtual void showContextMenu();

    QSize sizeHint() const;
    bool justHidden();

protected:
    MixDeviceWidget *_mdw;
    KMixDockWidget  *_dock;
    MixDevice       *_dockDevice;
	 QPushButton     *_showPanelBox;

    void mousePressEvent(QMouseEvent *e);
    void wheelEvent ( QWheelEvent * e );

private:
    QGridLayout* _layoutMDW;
    QFrame *_frame;
    QTime *_hideTimer;

private slots:
	 void showPanelSlot();

};

#endif

