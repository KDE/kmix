/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) Trever Fischer <tdfischer@fedoraproject.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef CONTROL_H
#define CONTROL_H

#include <QtCore/QObject>
#include <QtCore/QMap>

class ControlMonitor;
class QSignalMapper;

class OrgKdeKMixControlMonitorInterface;
namespace org {
    namespace kde {
        namespace KMix {
            typedef OrgKdeKMixControlMonitorInterface ControlMonitor;
        }
    }
}

class Control : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName);
    Q_PROPERTY(QString iconName READ iconName);
    Q_PROPERTY(bool mute READ isMuted WRITE setMute NOTIFY muteChanged);
    Q_PROPERTY(bool canMute READ canMute);
    Q_PROPERTY(int channels READ channels);
    Q_PROPERTY(Category category READ category);
    Q_PROPERTY(bool canMonitor READ canMonitor);
    Q_PROPERTY(bool canChangeTarget READ canChangeTarget);
    Q_PROPERTY(QStringList alternateTargets READ alternateTargets);
    Q_PROPERTY(QString currentTarget READ currentTarget);
    Q_PROPERTY(int id READ id);
public:
    typedef enum {
        FrontLeft,
        FrontRight,
        Center,
        RearLeft,
        RearRight,
        Subwoofer,
        SideLeft,
        SideRight,
        RearCenter,
        Mono
    } Channel;
    typedef enum {
        OutputStream,
        InputStream,
        HardwareInput,
        HardwareOutput
    } Category;
    explicit Control(Category, QObject *parent = 0);
    ~Control();

    virtual QString displayName() const = 0;
    virtual QString iconName() const = 0;
    virtual int channels() const = 0;
    virtual int getVolume(Channel c) const = 0;
    virtual void setVolume(Channel c, int v) = 0;
    virtual bool isMuted() const = 0;
    virtual void setMute(bool yes) = 0;
    virtual bool canMute() const = 0;
    virtual bool canMonitor() const;
    virtual void changeTarget(Control *t);
    virtual void stopMonitor();
    virtual void startMonitor();

    int id() const;
    Category category() const;

    int getVolume(int i) const {return getVolume((Channel)i);}
    void setVolume(int c, int v) {setVolume((Channel)c, v);}
    Q_INVOKABLE void subscribeMonitor(const QString &address, const QString &path);
    Q_INVOKABLE void setTarget(int id);

    QStringList alternateTargets() const;
    void addAlternateTarget(Control *t);
    void removeAlternateTarget(Control *t);
    bool canChangeTarget() const;
    QString currentTarget() const;
signals:
    void volumeChanged(int c);
    void muteChanged(bool muted);
    void removed();
    void currentTargetChanged(const QString &path);
    void alternateTargetAdded(const QString &path);
    void alternateTargetRemoved(const QString &path);
protected:
    void levelUpdate(int l);
    void setCurrentTarget(Control *t);
private slots:
    void removeMonitor(int);
private:
    Category m_category;
    int m_id;
    static QAtomicInt s_id;
    QList<org::kde::KMix::ControlMonitor*> m_monitors;
    QSignalMapper *m_monitorMap;
    QList<Control*> m_targets;
    Control *m_currentTarget;
};

#endif //CONTROL_H
