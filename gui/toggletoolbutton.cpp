/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2018 Jonathan Marten <jjm@keelhaul.me.uk>
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
 * <http://www.gnu.org/licenses>.
 */


#include "toggletoolbutton.h"

#include <qicon.h>
#include <qlabel.h>

#include <kiconloader.h>
#include <kiconeffect.h>

#include <kmix_debug.h>


static const KIconLoader::Group iconLoadGroup = KIconLoader::Small;
static const KIconLoader::Group iconSizeGroup = KIconLoader::Toolbar;
static const int iconSmallSize = 10;


ToggleToolButton::ToggleToolButton(const QString &activeIconName, QWidget *pnt)
    : QToolButton(pnt)
{
    mActiveLoaded = mInactiveLoaded = false;
    mActiveIcon = activeIconName;
    mSmallSize = false;
    mIsActive = true;
    mFirstTime = true;

    setCheckable(false);
    setAutoRaise(true);
}


// based on MDWSlider::setIcon()
QPixmap getPixmap(const QString &name, bool small = false)
{
    QPixmap pix = KIconLoader::global()->loadIcon(name, iconLoadGroup, IconSize(iconSizeGroup));
    if (!pix.isNull())					// load icon, check success
    {							// if wanting small size, scale pixmap
	if (small) pix = pix.scaled(iconSmallSize, iconSmallSize);
    }
    else qCWarning(KMIX_LOG) << "failed to load" << name;

    // Return the allocated pixmap even if it failed to load, so that
    // the caller can tell and only one load attempt will be made.
    return (pix);
}


void ToggleToolButton::setActive(bool active)
{
    if (!mFirstTime && (active==mIsActive)) return;	// no change required
    mIsActive = active;					// record required state
    mFirstTime = false;					// note now initialised

    QPixmap *pix = nullptr;				// new pixmap to set
    if (mIsActive)					// look at new state
    {
        if (mActivePixmap.isNull())			// need pixmap for active state
        {						// only if not already tried
            if (!mActiveLoaded) mActivePixmap = getPixmap(mActiveIcon, mSmallSize);
            mActiveLoaded = true;			// note not to try again
        }

        pix = &mActivePixmap;				// the pixmap to use
    }
    else						// want inactive state
    {
        if (mInactivePixmap.isNull())			// need pixmap for inactive state
        {
            if (!mInactiveIcon.isEmpty())		// inactive icon is set
            {
                if (!mInactiveLoaded) mInactivePixmap = getPixmap(mInactiveIcon, mSmallSize);
            }
            else
            {						// need to derive from active state
                if (!mActiveLoaded) mActivePixmap = getPixmap(mActiveIcon, mSmallSize);
                mActiveLoaded = true;			// only if not already tried
                if (mActivePixmap.isNull()) qCWarning(KMIX_LOG) << "want inactive but no active available";
                else
                {
                    mInactivePixmap = KIconLoader::global()->iconEffect()->apply(mActivePixmap,
                                                                                 KIconLoader::Toolbar,
                                                                                 KIconLoader::DisabledState);
                }
            }

            mInactiveLoaded = true;			// note not to try again
        }

        pix = &mInactivePixmap;				// the pixmap to use
    }

    if (pix->isNull()) return;				// pixmap not available
    setIcon(*pix);					// set button pixmap
}


/**
 * Loads the icon with the given @p iconName in the size KIconLoader::Small,
 * and applies it to the @p label widget.  The widget must be either a
 * QLabel or a QToolButton.
 *
 * Originally @c MDWSlider::setIcon(), moved here because it uses the same
 * icon size parameters as a @c ToggleToolButton, and it can share @c getPixmap().
 */
void ToggleToolButton::setIndicatorIcon(const QString &iconName, QWidget *label, bool small)
{
    QPixmap pix = getPixmap(iconName, small);
    if (pix.isNull())
    {
        qCWarning(KMIX_LOG) << "Could not get pixmap for" << iconName;
        return;
    }

    if (small)						// small size, set for scaled icon
    {
        label->resize(iconSmallSize, iconSmallSize);
    }
    else						// not small size, set for normal icon
    {
        label->resize(IconSize(iconSizeGroup), IconSize(iconSizeGroup));
    }
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		
    QLabel *lbl = qobject_cast<QLabel *>(label);
    if (lbl!=nullptr)
    {
        lbl->setPixmap(pix);
        lbl->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    }
    else
    {
        QToolButton *tbt = qobject_cast<QToolButton *>(label);
        if (tbt!=nullptr) tbt->setIcon(pix);		// works because implicit QPixmap -> QIcon
    }
}
