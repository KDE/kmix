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

class QComboBox;
class QVBoxLayout;
class QListWidget;

#include "dialogbase.h"

class Mixer;

class DialogSelectMaster : public DialogBase
{
    Q_OBJECT

 public:
    explicit DialogSelectMaster(const Mixer *mixer = nullptr, QWidget *parent = nullptr);
    virtual ~DialogSelectMaster() = default;

 public slots:
    void apply();

 private:
    void createWidgets(const Mixer *mixer);
    void createPage(const Mixer *mixer);

 private:
    QComboBox* m_cMixer;
    QListWidget *m_channelSelector;

 private slots:
    void createPageByID(int mixerId);
    void slotUpdateButtons();
};

#endif
