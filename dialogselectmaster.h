#ifndef DIALOGSELECTMASTER_H
#define DIALOGSELECTMASTER_H

class QButtonGroup;
#include <qradiobutton.h>
#include <qptrlist.h>
class QScrollView;
class QVBox;
class QVBoxLayout;

class KComboBox;
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
   void createWidgets(int);
    QVBoxLayout* _layout;
    //QPtrList<QRadioButton>  _qEnabledCB;
    KComboBox* m_cMixer;
    QScrollView* m_scrollableChannelSelector;
    QVBox *m_vboxForScrollView;
    QButtonGroup *m_buttonGroupForScrollView;
    
 private slots:
    void createPage(int mixerId);
};

#endif
