#ifndef ViewDockAreaPopup_h
#define ViewDockAreaPopup_h

#include "viewbase.h"

class QMouseEvent;
class QHBoxLayout;
class QWidget;

class Mixer;
class KMixDockWidget;
class MixDeviceWidget;
class MixDevice;

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
    MixDeviceWidget* getMdwHACK();

protected:
    MixDeviceWidget *_mdw;
    KMixDockWidget  *_dock;
    MixDevice       *_dockDevice;

    void mousePressEvent(QMouseEvent *e);
private:
    QHBoxLayout* _layoutMDW;

};

#endif

