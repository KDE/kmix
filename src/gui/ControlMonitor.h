#ifndef CONTROLMONITOR_H
#define CONTROLMONITOR_H

#include <QtCore/QObject>

class Control;
class QProgressBar;

class OrgKdeKMixControlInterface;
namespace org {
    namespace kde {
        namespace KMix {
            typedef OrgKdeKMixControlInterface Control;
        }
    }
}

class ControlMonitor : public QObject
{
    Q_OBJECT
public:
    ControlMonitor(QProgressBar *widget, org::kde::KMix::Control *control, QObject *parent);
    ~ControlMonitor();
public slots:
    void levelUpdate(int level);
signals:
    void removed();
private:
    QProgressBar *m_widget;
    static QAtomicInt s_id;
};

#endif // CONTROLMONITOR_H
