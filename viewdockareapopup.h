#ifndef ViewDockAreaPopup_h
#define ViewDockAreaPopup_h

#include "viewbase.h"
class QWidget;
class Mixer;
class KMixDockWidget;
class MixDeviceWidget;
class MixDevice;
class QMouseEvent;

#define KMIX_DOCKAREA_MPE 1
//#define KMIX_DOCKAREA_EF  1

class ViewDockAreaPopup : public ViewBase
{
    Q_OBJECT
public:
    ViewDockAreaPopup(QWidget* parent, const char* name, Mixer* mixer, KMixDockWidget *dockW);
    ~ViewDockAreaPopup();
    MixDevice* dockDevice();

    virtual int count();
    virtual int advice();
    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();
    virtual void refreshVolumeLevels();
    virtual void showContextMenu();

    QSize sizeHint();
    MixDeviceWidget* getMdwHACK();

protected:
    MixDeviceWidget *_mdw;
    KMixDockWidget  *_dock;
    MixDevice       *_dockDevice;

#ifdef KMIX_DOCKAREA_MPE
    void mousePressEvent(QMouseEvent *e);
#endif
private:
#ifdef KMIX_DOCKAREA_EF
    bool eventFilter( QObject*, QEvent* );
#endif
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
};

#endif

