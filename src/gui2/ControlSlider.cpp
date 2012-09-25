#include "ControlSlider.h"
#include "ControlMonitor.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSlider>
#include <QtCore/QSignalMapper>
#include <KDE/KIcon>
#include <QtGui/QPushButton>
#include <QtGui/QProgressBar>

#include "control_interface.h"

ControlSlider::ControlSlider(org::kde::KMix::Control *control, QWidget *parent)
    : QWidget(parent)
    , m_control(control)
    , m_mute(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    QWidget *labelContainer = new QWidget(this);
    QHBoxLayout *labelLayout = new QHBoxLayout(labelContainer);
    labelContainer->setLayout(labelLayout);

    QLabel *label = new QLabel(m_control->displayName(), labelContainer);
    QLabel *iconLabel = new QLabel(labelContainer);
    KIcon icon(control->iconName());
    iconLabel->setPixmap(icon.pixmap(QSize(32, 32)));

    labelLayout->addWidget(iconLabel);
    labelLayout->addWidget(label);

    QWidget *sliderContainer = new QWidget(this);
    QVBoxLayout *sliderLayout = new QVBoxLayout(sliderContainer);

    QSignalMapper *mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(updateVolume(int)));
    for(int i = 0;i<control->channels();i++) {
        QSlider *slider = new QSlider(sliderContainer);
        sliderLayout->addWidget(slider);
        slider->setMaximum(65536);
        slider->setValue(control->getVolume(i));
        slider->setOrientation(Qt::Horizontal);
        mapper->setMapping(slider, i);
        m_sliders << slider;
        connect(slider, SIGNAL(valueChanged(int)), mapper, SLOT(map()));
    }

    layout->addWidget(labelContainer);
    layout->addWidget(sliderContainer);
    if (control->canMonitor()) {
        QProgressBar *levelDisplay = new QProgressBar(this);
        levelDisplay->setMaximum(65536);
        new ControlMonitor(levelDisplay, control, levelDisplay);
        layout->addWidget(levelDisplay);
    }
    if (control->canMute()) {
        m_mute = new QPushButton(this);
        connect(m_mute, SIGNAL(clicked(bool)), this, SLOT(toggleMute()));
        layout->addWidget(m_mute);
    }

    updateMute();
    connect(control, SIGNAL(volumeChanged(int)), this, SLOT(volumeChange(int)));
    connect(control, SIGNAL(muteChanged(bool)), this, SLOT(updateMute()));
}

ControlSlider::~ControlSlider()
{
}

void ControlSlider::volumeChange(int channel)
{
    qDebug() << "Updated volume on" << channel << m_control->getVolume(channel);
    m_sliders[channel]->setValue(m_control->getVolume(channel));
}

void ControlSlider::updateVolume(int channel)
{
    m_control->setVolume(channel, m_sliders[channel]->value());
}

void ControlSlider::updateMute()
{
    if (m_mute) {
        KIcon icon;
        if (m_control->mute()) {
            icon = KIcon("audio-volume-muted");
        } else {
            icon = KIcon("audio-volume-high");
        }
        m_mute->setIcon(icon);
    }
}

void ControlSlider::toggleMute()
{
    if (m_mute) {
        m_control->setMute(!m_control->mute());
    }
}


#include "ControlSlider.moc"
