////-*-C++-*-
///*
// * KMix -- KDE's full featured mini mixer
// *
// *
// * Copyright Chrisitan Esken <esken@kde.org>
// *
// * This program is free software; you can redistribute it and/or
// * modify it under the terms of the GNU Library General Public
// * License as published by the Free Software Foundation; either
// * version 2 of the License, or (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// * Library General Public License for more details.
// *
// * You should have received a copy of the GNU Library General Public
// * License along with this program; if not, write to the Free
// * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// */
//
//#ifndef MDWSWITCH_H
//#define MDWSWITCH_H
//
//#include <QWidget>
//#include "core/volume.h"
//#include <qpixmap.h>
//
//class QBoxLayout;
//class QLabel;
//
//class QCheckBox;
//class KAction;
//
//class MixDevice;
//class VerticalText;
//class Mixer;
//class ViewBase;
//
//#include "gui/mixdevicewidget.h"
//
//class MDWSwitch : public MixDeviceWidget
//{
//    Q_OBJECT
//
//public:
//   MDWSwitch( MixDevice* md,
//   bool small, Qt::Orientation orientation,
//   QWidget* parent = 0, ViewBase* mw = 0);
//   ~MDWSwitch();
//
//   void addActionToPopup( KAction *action );
//   QSizePolicy sizePolicy() const;
//   void setBackgroundRole(QPalette::ColorRole m);
//   bool eventFilter( QObject* obj, QEvent* e );
//
//public slots:
//   // GUI hide and show
//   void setDisabled();
//   void setDisabled(bool);
//
//   // Switch on/off
//   void toggleSwitch();
//   void setSwitch(bool value);
//
//   void update();
//   virtual void showContextMenu();
//
//private:
//   void createWidgets();
//
//   QLabel        *_label;
//   VerticalText  *_labelV;
//   QCheckBox    *_switchLED;
//   QBoxLayout    *_layout;
//};
//
//#endif
