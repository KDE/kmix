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
 * <https://www.gnu.org/licenses>.
 */


#include "toggletoolbutton.h"

#include <qicon.h>
#include <qlabel.h>
#include <qiconengine.h>
#include <qstyle.h>

#include <kmix_debug.h>


static const int iconSmallSize = 10;


class DisabledIconEngine : public QIconEngine
{
    QIcon m_icon;
public:
    DisabledIconEngine(const QIcon &icon) : QIconEngine(), m_icon(icon) {}
    ~DisabledIconEngine() override {}

    QIconEngine *clone() const override
    {
        return new DisabledIconEngine(m_icon);
    }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode, QIcon::State state) override
    {
        return m_icon.paint(painter, rect, Qt::AlignCenter, QIcon::Disabled, state);
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode, QIcon::State state) override
    {
        return m_icon.pixmap(size, QIcon::Disabled, state);
    }

    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        return m_icon.actualSize(size, mode, state);
    }

    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) const override
    {
        return m_icon.availableSizes(mode, state);
    }
};


ToggleToolButton::ToggleToolButton(const QString &activeIconName, QWidget *pnt)
    : QToolButton(pnt)
{
    mActiveLoaded = mInactiveLoaded = false;
    mActiveIconName = activeIconName;
    mSmallSize = false;
    mIsActive = true;
    mFirstTime = true;

    setCheckable(false);
    setAutoRaise(true);
}


void ToggleToolButton::setActive(bool active)
{
    if (!mFirstTime && (active==mIsActive)) return;	// no change required
    mIsActive = active;					// record required state
    mFirstTime = false;					// note now initialised

    QIcon icon;						// new icon to set
    if (mIsActive)					// look at new state
    {
        if (mActiveIcon.isNull())			// need icon for active state
        {						// only if not already tried
            if (!mActiveLoaded) mActiveIcon = QIcon::fromTheme(mActiveIconName);
            mActiveLoaded = true;			// note not to try again
        }

        icon = mActiveIcon;				// the icon to use
    }
    else						// want inactive state
    {
        if (mInactiveIcon.isNull())			// need icon for inactive state
        {
            if (!mInactiveIconName.isEmpty())		// inactive icon is set
            {
                if (!mInactiveLoaded) mInactiveIcon = QIcon::fromTheme(mInactiveIconName);
            }
            else
            {						// need to derive from active state
                if (!mActiveLoaded) mActiveIcon = QIcon::fromTheme(mActiveIconName);
                mActiveLoaded = true;			// only if not already tried
                if (mActiveIcon.isNull()) qCWarning(KMIX_LOG) << "want inactive but no active available";
                else
                {
                    mInactiveIcon = QIcon(new DisabledIconEngine(mActiveIcon));
                }
            }

            mInactiveLoaded = true;			// note not to try again
        }

        icon = mInactiveIcon;				// the icon to use
    }

    if (icon.isNull()) return;				// icon not available
    setIcon(icon);					// set button icon
}


/**
 * Loads the icon with the given @p iconName
 * and applies it to the @p label widget.  The widget must be either a
 * QLabel or a QToolButton.
 *
 * Originally @c MDWSlider::setIcon(), moved here because it uses the same
 * icon size parameters as a @c ToggleToolButton, and it can share @c getPixmap().
 */
void ToggleToolButton::setIndicatorIcon(const QString &iconName, QWidget *label, bool small)
{
    const QStringList iconNames = iconName.split(';');
    QIcon icon;
    for (const auto &name : iconNames)
    {
        if (!QIcon::hasThemeIcon(name)) continue;
        icon = QIcon::fromTheme(name);
        break;
    }

    if (icon.isNull())
    {
        qCWarning(KMIX_LOG) << "Could not get icon for" << iconName;
        return;
    }

    QSize iconSize;
    if (small)						// small size, set for scaled icon
    {
        iconSize = QSize(iconSmallSize, iconSmallSize);
    }
    else						// not small size, set for normal icon
    {
        const auto toolBarIconSize = label->style()->pixelMetric(QStyle::PM_TabBarIconSize, nullptr, label);
        iconSize = QSize(toolBarIconSize, toolBarIconSize);
    }
    label->resize(iconSize);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		
    QLabel *lbl = qobject_cast<QLabel *>(label);
    if (lbl!=nullptr)
    {
        lbl->setPixmap(icon.pixmap(iconSize));
        lbl->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    }
    else
    {
        QToolButton *tbt = qobject_cast<QToolButton *>(label);
        if (tbt!=nullptr) tbt->setIcon(icon);
    }
}
