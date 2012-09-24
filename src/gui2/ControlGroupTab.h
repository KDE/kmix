#ifndef CONTROLGROUPTAB_H
#define CONTROLGROUPTAB_H

#include <QtGui/QWidget>

class OrgKdeKMixControlGroupInterface;
namespace org {
    namespace kde {
        namespace KMix {
            typedef OrgKdeKMixControlGroupInterface ControlGroup;
        }
    }
}

class ControlGroupTab : public QWidget {
    Q_OBJECT
public:
    ControlGroupTab(org::kde::KMix::ControlGroup *group, QWidget *parent);
    ~ControlGroupTab();
private:
    org::kde::KMix::ControlGroup *m_group;
};

#endif // CONTROLGROUPTAB_H
