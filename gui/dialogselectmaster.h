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
#ifndef DIALOGSELECTMASTER_H
#define DIALOGSELECTMASTER_H

class QButtonGroup;
class KComboBox;
#include <qradiobutton.h>
class QScrollArea;
#include <kvbox.h>
class QVBoxLayout;

#include <kdialog.h>

class Mixer;

class DialogSelectMaster : public KDialog
{
    Q_OBJECT
 public:
    DialogSelectMaster(Mixer * = 0);
    ~DialogSelectMaster();

 public slots:
    void apply();

 private:
    void createWidgets(Mixer*);
    void createPage(Mixer*);
    QVBoxLayout* _layout;
    KComboBox* m_cMixer;
    QScrollArea* m_scrollableChannelSelector;
    KVBox *m_vboxForScrollView;
    QButtonGroup *m_buttonGroupForScrollView;
    //QStringList m_mixerPKs;
    QFrame *m_mainFrame;

 private slots:
   void createPageByID(int mixerId);
};

#endif
