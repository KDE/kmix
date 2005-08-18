#ifndef ViewInput_h
#define ViewInput_h

#include "viewsliders.h"
class QWidget;
class Mixer;

class ViewInput : public ViewSliders
{
    Q_OBJECT
public:
    ViewInput(QWidget* parent, const char* name, const QString & caption, Mixer* mixer, ViewBase::ViewFlags vflags);
    ~ViewInput();

    virtual void setMixSet(MixSet *mixset);
};

#endif

