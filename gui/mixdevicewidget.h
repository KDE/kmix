//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 *               1996-2000 Christian Esken <esken@kde.org>
 *                         Sven Fischer <herpes@kawo2.rwth-aachen.de>
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

#ifndef MIXDEVICEWIDGET_H
#define MIXDEVICEWIDGET_H

#include "core/mixdevice.h"
#include "core/volume.h"
#include "gui/viewbase.h"

class KActionCollection;
class KShortcutsDialog;

class MixDevice;
class ProfControl;

class MixDeviceWidget : public QWidget
{
    Q_OBJECT

public:
    enum MDWFlag
    {
        ShowMute = 0x02,
        ShowCapture = 0x04,
        ShowMixerName = 0x08
    };
    Q_DECLARE_FLAGS(MDWFlags, MDWFlag)

    MixDeviceWidget(shared_ptr<MixDevice> md, MDWFlags flags, ViewBase *view, ProfControl *pctl);
    virtual ~MixDeviceWidget() = default;

    shared_ptr<MixDevice> mixDevice() const		{ return (m_mixdevice); }

    virtual void setColors( QColor high, QColor low, QColor back );
    virtual void setIcons( bool value );
    virtual void setMutedColors( QColor high, QColor low, QColor back );

    virtual bool isStereoLinked() const			{ return (false); }
    virtual void setStereoLinked(bool)			{}
    virtual void setLabeled(bool);
    virtual void setTicks(bool)				{}

    virtual int labelExtentHint() const			{ return (0); }
    virtual void setLabelExtent(int extent)		{ Q_UNUSED(extent); }

public slots:
    /**
      * Called whenever there are volume updates pending from the hardware for this MDW.
      */
    virtual void update() = 0;

signals:
    void guiVisibilityChange(MixDeviceWidget* source, bool enable);

protected slots:
    void volumeChange(int);

protected:
    virtual void createContextMenu(QMenu *menu) = 0;
    void contextMenuEvent(QContextMenuEvent *ev) override;

    Qt::Orientation orientation() const			{ return (m_view->orientation()); }
    MixDeviceWidget::MDWFlags flags() const		{ return (m_flags); }
    ViewBase *view() const				{ return (m_view); }
    ProfControl *profileControl() const			{ return (m_pctl); }

    KActionCollection *channelActions() const		{ return (m_channelActions); }
    KActionCollection *globalActions() const		{ return (m_globalActions); }

private slots:
    void configureShortcuts();

private:
      MDWFlags m_flags;
      shared_ptr<MixDevice>  m_mixdevice;
      ProfControl *m_pctl;
      ViewBase *m_view;
      KShortcutsDialog *m_shortcutsDialog;

      KActionCollection *m_channelActions;
      KActionCollection *m_globalActions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MixDeviceWidget::MDWFlags)

#endif
