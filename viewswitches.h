#ifndef ViewSwitches_h
#define ViewSwitches_h

class QLayout;
class QWidget;

class Mixer;
#include "viewbase.h"

class ViewSwitches : public ViewBase
{
    Q_OBJECT
public:
    ViewSwitches(QWidget* parent, const char* name, const QString & caption, Mixer* mixer, ViewBase::ViewFlags vflags);
    ~ViewSwitches();

    virtual int count();
    virtual int advice();
    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void constructionFinished();
    virtual void configurationUpdate();

    QSize sizeHint() const;

public slots:
    virtual void refreshVolumeLevels();

private:
    QLayout* _layoutMDW;
    QLayout* _layoutEnum;
    QLayout* _layoutSwitch;
};

#endif

