/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2017 Jonathan Marten <jjm@keelhaul.me.uk>
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
 * License along with this program; if not, see
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef DIALOGSTATESAVER_H
#define DIALOGSTATESAVER_H

#include <qobject.h>

class QDialog;
class QEvent;
class QAbstractButton;
class KConfigGroup;


/**
 * @short A helper to save and restore the size and state of a dialog box.
 *
 * The helper takes care of saving and restoring a dialog's size in the
 * application config file.  All that is necessary is to create a
 * DialogStateSaver object in the dialog's constructor, passing the
 * dialog as a parameter.  If the dialog is a subclass of DialogBase
 * then a saver will be created automatically.
 *
 * @author Jonathan Marten
 **/

class DialogStateSaver : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param pnt the parent dialog
     **/
    explicit DialogStateSaver(QDialog *pnt);

    /**
     * Destructor.
     **/
    virtual ~DialogStateSaver() = default;

    /**
     * Set the default option of whether the size of dialog boxes
     * is saved when accepted and restored when shown.  This is an
     * application-wide setting which takes effect immediately.
     * The default is @c true.
     *
     * @param on Whether the size is to be saved/restored
     *
     * @note The setting for each dialog is saved separately in the application's
     * default configuration file (as used by @c KSharedConfig::openConfig())
     * in a section named by the dialog's object name.  If no object name is
     * set then the dialog's class name is used.
     *
     * @see KSharedConfig
     * @see QObject::objectName()
     **/
    static void setSaveSettingsDefault(bool on);

    /**
     * Sets a button to save the state of the dialog when it is used.
     *
     * Normally the dialog state will be saved when the parent dialog is accepted.
     * This means when any button with the @c QDialogButtonBox::AcceptRole is
     * clicked: that is, @c QDialogButtonBox::Ok and some others.  Notably, it
     * does not include the @c QDialogButtonBox::Close button which is used where
     * there is no difference between closing and cancelling.  This means that the
     * dialog state will not normally be saved when that button is used.
     *
     * If a button is specified here, the state will be saved when that button is
     * used, in addition to any button with the @c QDialogButtonBox::AcceptRole.
     * Additional buttons may be specified multiple times, and they will all
     * save the state.
     *
     * @param but The button to activate the saving
     */
    void setSaveOnButton(QAbstractButton *but);

    /**
     * Save the state of a window.
     *
     * The window need not be a dialog, therefore this can be used for
     * saving the state of any window.  The state is saved to a group
     * named as appropriate for the window.
     *
     * @param widget window to save the state of
     **/
    static void saveWindowState(QWidget *widget);

    /**
     * Save the state of a window.
     *
     * The window need not be a dialog, therefore this can be used for
     * saving the state of any window.  The state is saved to the
     * specified group.
     *
     * @param widget window to save the state of
     * @param grp group to save the configuration to
     **/
    static void saveWindowState(QWidget *widget, KConfigGroup &grp);

    /**
     * Restore the state of a window.
     *
     * The window need not be a dialog, therefore this can be used for
     * restoring the state of any window.  The state is restored from
     * a group named as appropriate for the window.
     *
     * @param widget window to restore the state of
     **/
    static void restoreWindowState(QWidget *widget);

    /**
     * Restore the state of a window.
     *
     * The window need not be a dialog, therefore this can be used for
     * restoring the state of any window.  The state is restored from
     * the specified group.
     *
     * @param widget window to restore the state of
     * @param grp group to restore the configuration from
     **/
    static void restoreWindowState(QWidget *widget, const KConfigGroup &grp);

protected:
    /**
     * Save the dialog size to the application config file.
     *
     * This may be reimplemented in a subclass if necessary, in order
     * to save other settings (e.g. the column states of a list view).
     * Call the base class implementation to save the dialog size.
     *
     * @param dialog dialog to save the state of
     * @param grp group to save the configuration to
     **/
    virtual void saveConfig(QDialog *dialog, KConfigGroup &grp) const;

    /**
     * Restore the dialog size from the application config file.
     *
     * This may be reimplemented in a subclass if necessary, in order
     * to restore other settings (e.g. the column states of a list view).
     * Call the base class implementation to restore the dialog size.
     *
     * @param dialog dialog to restore the state of
     * @param grp group to restore the configuration from
     **/
    virtual void restoreConfig(QDialog *dialog, const KConfigGroup &grp);

    /**
     * @reimp
     **/
    virtual bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void restoreConfigInternal();
    void saveConfigInternal() const;

private:
    QDialog *mParent;
};

#endif							// DIALOGSTATESAVER_H
