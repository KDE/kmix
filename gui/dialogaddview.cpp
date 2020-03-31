/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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

#include "gui/dialogaddview.h"

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>

#include <klocalizedstring.h>

#include "core/mixer.h"
#include "gui/kmixtoolbox.h"


static QStringList viewNames;
static QStringList viewIds;


DialogAddView::DialogAddView(QWidget *parent, const Mixer *mixer)
    : DialogBase(parent)
{
	// TODO 000 Adding View for MPRIS2 is broken. We need at least a dummy XML GUI Profile. Also the
	//      fixed list below is plain wrong. Actually we should get the Profile list from either the XML files or
	//      from the backend. The latter is probably easier for now.
    if ( viewNames.isEmpty() )
    {
        // Initialize static list.
        // Maybe this list could be generated from the actually installed profiles.
        viewNames.append(i18n("All controls"));
        viewNames.append(i18n("Only playback controls"));
        viewNames.append(i18n("Only capture controls"));

        viewIds.append("default");
        viewIds.append("playback");
        viewIds.append("capture");
    }

    setWindowTitle(i18n("Add View"));
    if (!Mixer::mixers().isEmpty()) setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    else setButtons(QDialogButtonBox::Cancel);

    m_listForChannelSelector = nullptr;
    createWidgets(mixer);				// for the specified 'mixer'
}


/**
 * Create basic widgets of the dialogue.
 */
void DialogAddView::createWidgets(const Mixer *mixer)
{
    QWidget *mainFrame = new QWidget(this);
    setMainWidget(mainFrame);
    QVBoxLayout *layout = new QVBoxLayout(mainFrame);

    const QList<Mixer *> &mixers = Mixer::mixers();	// list of all mixers present
    int mixerIndex = 0;					// index of specified 'mixer'

    if (mixers.count()>1)
    {
        // More than one Mixer => show Combo-Box to select Mixer
        // Mixer widget line
        QHBoxLayout* mixerNameLayout = new QHBoxLayout();
        layout->addLayout(mixerNameLayout);
        mixerNameLayout->setSpacing(DialogBase::horizontalSpacing());
    
        QLabel *qlbl = new QLabel( i18n("Select mixer:"), mainFrame );
        mixerNameLayout->addWidget(qlbl);
        qlbl->setFixedHeight(qlbl->sizeHint().height());
    
        m_cMixer = new QComboBox(mainFrame);
        m_cMixer->setObjectName( QLatin1String( "mixerCombo" ) );
        m_cMixer->setFixedHeight(m_cMixer->sizeHint().height());
        m_cMixer->setToolTip(i18n("Current mixer"));

        for (int i = 0; i<mixers.count(); ++i)
        {
            const Mixer *m = mixers[i];
            const QString name = m->readableName();
            m_cMixer->addItem(QIcon::fromTheme(m->iconName()), name);
            if (name==mixer->readableName()) mixerIndex = i;
        }

        // Select the specified 'mixer' as the current item in the combo box.
        // If it was not found by the loop above, then select the first item.
        m_cMixer->setCurrentIndex(mixerIndex);
        
        connect(m_cMixer, QOverload<int>::of(&QComboBox::activated), this, &DialogAddView::createPageByID);
        mixerNameLayout->addWidget(m_cMixer);
    }

    if (mixers.count()>0)
    {
        QLabel *qlbl = new QLabel( i18n("Select the design for the new view:"), mainFrame );
        layout->addWidget(qlbl);

        createPage(mixer);				// equivalent to mixers[mixerIndex]

        connect(this, &QDialog::accepted, this, &DialogAddView::apply);
    }
    else
    {
	QWidget *noMixersWarning = KMixToolBox::noDevicesWarningWidget(mainFrame);
        layout->addWidget(noMixersWarning);
        layout->addStretch(1);
    }
}


/**
 * Create the view profile list for the specified mixer
 *
 * @p mixerId the ID (index) of the mixer for which the list should be created.
 */
void DialogAddView::createPageByID(int mixerId)
{
    Mixer *mixer = Mixer::mixers().at(mixerId);
    if (mixer!=nullptr) createPage(mixer);
}


/**
 * Create the view profile list for the specified mixer
 *
 * @p mixer the mixer for which the list should be created
 */
void DialogAddView::createPage(const Mixer *mixer)
{
    /** --- Reset page -----------------------------------------------
     * In case the user selected a new Mixer via m_cMixer, we need
     * to remove the stuff created on the last call.
     */
    delete m_listForChannelSelector;
    setButtonEnabled(QDialogButtonBox::Ok, false);

        /** Reset page end -------------------------------------------------- */
    
    QWidget *mainFrame = mainWidget();
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(mainFrame->layout());
    Q_ASSERT(layout!=nullptr);

    m_listForChannelSelector = new QListWidget(mainFrame);
    m_listForChannelSelector->setUniformItemSizes(true);
    m_listForChannelSelector->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_listForChannelSelector, &QListWidget::itemSelectionChanged, this, &DialogAddView::profileSelectionChanged);
    layout->addWidget(m_listForChannelSelector);

    for (int i = 0; i<viewNames.size(); ++i)
    {
    	QString viewId = viewIds.at(i);
    	if (viewId != "default" && mixer->isDynamic())
    	{
    		// TODO: The mixer's backend MUST be inspected to find out the supported profiles.
    		//       Hardcoding it here is only a quick workaround.
    		continue;
		}
		
        // Create an item for each view type
        QString name = viewNames.at(i);
        QListWidgetItem *item = new QListWidgetItem(name, m_listForChannelSelector);
        item->setData(Qt::UserRole, viewIds.at(i));  // mixer ID as data
    }

    // If there is only one option available to select, then preselect it.
    if (m_listForChannelSelector->count()==1) m_listForChannelSelector->setCurrentRow(0);
}


void DialogAddView::profileSelectionChanged()
{
    const QList<QListWidgetItem *> items = m_listForChannelSelector->selectedItems();
    setButtonEnabled(QDialogButtonBox::Ok, !items.isEmpty());
}


void DialogAddView::apply()
{
    const QList<Mixer *> &mixers = Mixer::mixers();	// list of all mixers present
    Mixer *mixer = nullptr;				// selected mixer found

    if (mixers.count()==1)
    {
        // only one mixer => no combo box => take first entry
        mixer = mixers.first();
    }
    else if (mixers.count()>1)
    {
        // use the mixer that is currently selected in the combo box
        const int idx = m_cMixer->currentIndex();
        if (idx!=-1) mixer = mixers.at(idx);
    }
    Q_ASSERT(mixer!=nullptr);

    QList<QListWidgetItem *> items = m_listForChannelSelector->selectedItems();
    if (items.isEmpty()) return;			// nothing selected
    QListWidgetItem *item = items.first();
    const QString viewName = item->data(Qt::UserRole).toString();

    qCDebug(KMIX_LOG) << "Result view name" << viewName << "for mixer" << mixer->id();
    resultMixerId = mixer->id();
    resultViewName = viewName;
}
