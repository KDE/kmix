#ifndef ViewSliders_h
#define ViewSliders_h

class QHBoxLayout;
class QWidget;

class Mixer;
#include "viewbase.h"

class ViewSliders : public ViewBase
{
    Q_OBJECT
public:
    ViewSliders(QWidget* parent, const char* name, Mixer* mixer, ViewBase::ViewFlags vflags);
    ~ViewSliders();

    virtual int count();
    virtual int advice();
    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();

    QSize sizeHint() const;

public slots:
    virtual void refreshVolumeLevels();

private:
    QHBoxLayout* _layoutMDW;
};

#endif

