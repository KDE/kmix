#ifndef ViewSurround_h
#define ViewSurround_h

class QHBoxLayout;
class QGridLayout;
class QWidget;

class MixDevice;
class MixDeviceWidget;
class Mixer;
#include "viewbase.h"

class ViewSurround : public ViewBase
{
    Q_OBJECT
public:
    ViewSurround(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags);
    ~ViewSurround();

    virtual int count();
    virtual int advice();
    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();

    QSize sizeHint() const;

public slots:
    virtual void refreshVolumeLevels();

private:
    MixDeviceWidget* createMDW(MixDevice *md, bool small, Qt::Orientation orientation);
    MixDevice *_mdSurroundFront;
    MixDevice *_mdSurroundBack;

    QHBoxLayout* _layoutMDW;
    QHBoxLayout* _layoutSliders;
    QGridLayout* _layoutSurround;
};

#endif

