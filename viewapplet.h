#ifndef ViewApplet_h
#define ViewApplet_h

#include "viewbase.h"
#include <kpanelapplet.h>

class QBoxLayout;
class QHBox;
class QSize;

class Mixer;

class ViewApplet : public ViewBase
{
    Q_OBJECT
public:
    ViewApplet(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags, KPanelApplet::Position pos);
    ~ViewApplet();

    virtual int count();
    virtual int advice();
    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();
    virtual void configurationUpdate();

    QSize       sizeHint() const;
    QSizePolicy sizePolicy() const;
    virtual void resizeEvent(QResizeEvent*);

signals:
    void appletContentChanged();

public slots:
   virtual void refreshVolumeLevels();

private:
    bool shouldShowIcons(QSize);
    QBoxLayout*   _layoutMDW;
    // Position of the applet (pLeft, pRight, pTop, pBottom)
    //KPanelApplet::Position  _KMIXposition;
    // Orientation of the applet (horizontal or vertical)
    Qt::Orientation _viewOrientation;
};

#endif

