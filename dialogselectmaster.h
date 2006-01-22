#ifndef DIALOGSELECTMASTER_H
#define DIALOGSELECTMASTER_H

class QButtonGroup;
#include <qradiobutton.h>
class QScrollView;
#include <qstringlist.h>
class QVBox;
class QVBoxLayout;

class KComboBox;
#include <kdialogbase.h>

class Mixer;

class DialogSelectMaster : public KDialogBase
{
    Q_OBJECT
 public:
    DialogSelectMaster(Mixer *);
    ~DialogSelectMaster();

 signals:
    void newMasterSelected(int, QString& );

 public slots:
    void apply();

 private:
    void createWidgets(Mixer*);
    void createPage(Mixer*);
    QVBoxLayout* _layout;
    KComboBox* m_cMixer;
    QScrollView* m_scrollableChannelSelector;
    QVBox *m_vboxForScrollView;
    QButtonGroup *m_buttonGroupForScrollView;
    QStringList m_mixerPKs;

 private slots:
   void createPageByID(int mixerId);
};

#endif
