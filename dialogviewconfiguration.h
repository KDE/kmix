#ifndef DIALOGVIEWCONFIGURATION_H
#define DIALOGVIEWCONFIGURATION_H

#include <qcheckbox.h>
#include <qptrlist.h>
class QVBoxLayout;

#include <kdialogbase.h>

#include "viewbase.h"


class DialogViewConfiguration : public KDialogBase
{
    Q_OBJECT
 public:
    DialogViewConfiguration(QWidget* parent, ViewBase& view);
    ~DialogViewConfiguration();

    QSize sizeHint() const;
 public slots:
    void apply();

 private:
    QVBoxLayout* _layout;
    ViewBase&    _view;
    QPtrList<QCheckBox>  _qEnabledCB;
};

#endif
