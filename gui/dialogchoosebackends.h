//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
#ifndef DIALOGCHOOSEBACKENDS_H
#define DIALOGCHOOSEBACKENDS_H

class QButtonGroup;

#include <qcheckbox.h>
#include <QList>
class QScrollArea;
class QVBoxLayout;

class KComboBox;
#include <kdialog.h>
#include <kvbox.h>

class Mixer;

class DialogChooseBackends : public KDialog
{
    Q_OBJECT
 public:
    DialogChooseBackends(QSet<QString>& backends);
    ~DialogChooseBackends();

 public slots:
    void apply();

 private:
    void createWidgets(QSet<QString>& backends);
    void createPage(QSet<QString>& backends);
    QVBoxLayout* _layout;
    QScrollArea* m_scrollableChannelSelector;
    KVBox *m_vboxForScrollView;
    QButtonGroup *m_buttonGroupForScrollView;
    QList<QCheckBox*> checkboxes;
    QFrame *m_mainFrame;
};

#endif
