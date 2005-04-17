#ifndef DIALOGSELECTMASTER_H
#define DIALOGSELECTMASTER_H

#include <qradiobutton.h>
#include <qptrlist.h>
class QVBoxLayout;

#include <kdialogbase.h>


class DialogSelectMaster : public KDialogBase
{
    Q_OBJECT
 public:
    DialogSelectMaster(QWidget* parent);
    ~DialogSelectMaster();

 signals:
    void newMasterSelected(int, int);

 public slots:
    void apply();

 private:
    void createPage(int mixerId);
    QVBoxLayout* _layout;
    QPtrList<QRadioButton>  _qEnabledCB;
};

#endif
