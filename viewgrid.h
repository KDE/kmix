#ifndef ViewGrid_h
#define ViewGrid_h

class QBoxLayout;
#include "qsize.h"
class QWidget;

class Mixer;
#include "viewbase.h"

class ViewGrid : public ViewBase
{
    Q_OBJECT
public:
    ViewGrid(QWidget* parent, const char* name, const QString & caption, Mixer* mixer, ViewBase::ViewFlags vflags);
    ~ViewGrid();

    virtual int count();
    virtual int advice();
    virtual void setMixSet(MixSet *mixset);
    virtual QWidget* add(MixDevice *mdw);
    virtual void configurationUpdate();
    virtual void constructionFinished();

    QSize sizeHint() const;

public slots:
    virtual void refreshVolumeLevels();

private:
    unsigned int m_spacingHorizontal;
    unsigned int m_spacingVertical;

    // m_maxX and m_maxY are the highest coordiantes encountered
    QSize m_sizeHint;
    
    unsigned int m_testingX;
    unsigned int m_testingY;
};

#endif

