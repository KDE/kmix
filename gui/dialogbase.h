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

#ifndef DIALOGBASE_H
#define DIALOGBASE_H

#include <qdialog.h>
#include <qdialogbuttonbox.h>

class QSpacerItem;
class QVBoxLayout;
class KGuiItem;
class KConfigGroup;
class DialogStateSaver;


/**
 * @short A wrapper for QDialog incorporating some convenience functions.
 *
 * This is a lightweight wrapper around QDialog, incorporating some useful
 * functions which used to be provided by KDialog in KDE4.  These are:
 *
 * - Managing the button box and providing access to its buttons
 * - Managing the top level layout
 * - Saving and restoring the dialog size
 *
 * @author Jonathan Marten
 **/

class DialogBase : public QDialog
{
    Q_OBJECT

public:
    /**
     * Destructor.
     *
     **/
    virtual ~DialogBase() = default;

    /**
     * Retrieve the main widget.
     *
     * @return the main widget
     **/
    QWidget *mainWidget() const				{ return (mMainWidget); }

    /**
     * Get a spacing hint suitable for use within the dialog layout.
     *
     * @return The spacing hint
     * @deprecated Kept for compatibility with KDE4.
     * Use @c verticalSpacing() or @c horizontalSpacing() as appropriate.
     **/
    static Q_DECL_DEPRECATED int spacingHint();

    /**
     * Get a vertical spacing suitable for use within the dialog layout.
     *
     * @return The spacing hint
     **/
    static int verticalSpacing();

    /**
     * Get a horizontal spacing suitable for use within the dialog layout.
     *
     * @return The spacing hint
     **/
    static int horizontalSpacing();

    /**
     * Create a spacer item suitable for use within a vertical layout.
     *
     * @return The spacer item
     **/
    static QSpacerItem *verticalSpacerItem();

    /**
     * Create a spacer item suitable for use within a horizontal layout.
     *
     * @return The spacer item
     **/
    static QSpacerItem *horizontalSpacerItem();

    /**
     * Access the dialog's button box.
     *
     * @return the button box
     **/
    QDialogButtonBox *buttonBox() const			{ return (mButtonBox); }

    /**
     * Set the standard buttons to be displayed within the button box.
     *
     * @param buttons The buttons required
     *
     * @note This can be called at any time and the buttons will change
     * accordingly.  However, the buttons will be regenerated which means
     * that any special button text or icons, or any signal connections from
     * them, will be lost.
     **/
    void setButtons(QDialogButtonBox::StandardButtons buttons);

    /**
     * Set the enable state of a button.
     *
     * @param button The button to set
     * @param state The enable state for the button
     **/
    void setButtonEnabled(QDialogButtonBox::StandardButton button, bool state = true);

    /**
     * Set the text of a button.
     *
     * @param button The button to set
     * @param state The new text for the button
     *
     * @note This can be called at any time, and the button will change
     * accordingly.
     **/
    void setButtonText(QDialogButtonBox::StandardButton button, const QString &text);

    /**
     * Set the icon of a button.
     *
     * @param button The button to set
     * @param state The new icon for the button
     *
     * @note This can be called at any time, and the button will change
     * accordingly.
     **/
    void setButtonIcon(QDialogButtonBox::StandardButton button, const QIcon &icon);

    /**
     * Set up a button from a @c KGuiItem.
     *
     * @param button The button to set
     * @param guiItem The @c KGuiItem for the button
     *
     * @note This can be called at any time, and the button will change
     * accordingly.
     **/
    void setButtonGuiItem(QDialogButtonBox::StandardButton button, const KGuiItem &guiItem);

    /**
     * Set a state saver for the dialog.
     *
     * This may be a subclass of a DialogStateSaver, reimplemented in
     * order to save special dialog settings (e.g. the column states of
     * a list view).  If this is not set then a default DialogStateSaver
     * will be created and used internally.  If a NULL state saver is
     * set explicitly using this function, then no state restoring or
     * saving will be done.
     *
     * @param saver the state saver
     *
     * @note The saver should be set before the dialog is shown for
     * the first time.
     * @note The DialogBase will take ownership of the specified
     * state saver, and delete it when finished.
     * @see DialogStateSaver
     **/
    void setStateSaver(DialogStateSaver *saver);

    /**
     * Access the state saver used by the dialog.
     *
     * This may be the default one, or that set by @c setStateSaver().
     *
     * @return the state saver
     **/
    DialogStateSaver *stateSaver() const 		{ return (mStateSaver); }

protected:
    /**
     * Constructor.
     *
     * @param pnt Parent widget
     **/
    explicit DialogBase(QWidget *pnt = nullptr);

    /**
     * Set the main widget to be displayed within the dialog.
     *
     * @param w The widget
     **/
    void setMainWidget(QWidget *w);

private:
    QVBoxLayout *mMainLayout;
    QDialogButtonBox *mButtonBox;
    QWidget *mMainWidget;
    DialogStateSaver *mStateSaver;
};

#endif							// DIALOGBASE_H
