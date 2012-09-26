#ifndef CONTROLGROUPTAB_H
#define CONTROLGROUPTAB_H

#include <QtGui/QWidget>
#include <QtCore/QHash>

class QLayout;
class ControlSlider;
class QResizeEvent;
class QLabel;

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
protected:
    void resizeEvent(QResizeEvent *evt);
private slots:
    void controlAdded(const QString &path);
    void controlRemoved(const QString &path);
private:
    org::kde::KMix::ControlGroup *m_group;
    QLayout *m_layout;
    QHash<QString, ControlSlider*> m_controls;
    QLabel *m_emptyLabel;
};

#endif // CONTROLGROUPTAB_H
