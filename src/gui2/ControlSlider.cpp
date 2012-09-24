#include "ControlSlider.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSlider>
#include <QtCore/QSignalMapper>
#include <KDE/KIcon>

#include "control_interface.h"

ControlSlider::ControlSlider(org::kde::KMix::Control *control, QWidget *parent)
    : QWidget(parent)
    , m_control(control)
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
    QHBoxLayout *sliderLayout = new QHBoxLayout(sliderContainer);

    QSignalMapper *mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(updateVolume(int)));
    for(int i = 0;i<control->channels();i++) {
        QSlider *slider = new QSlider(sliderContainer);
        sliderLayout->addWidget(slider);
        slider->setMaximum(65536);
        slider->setValue(control->getVolume(i));
        mapper->setMapping(slider, i);
        m_sliders << slider;
        connect(slider, SIGNAL(valueChanged(int)), mapper, SLOT(map()));
    }
    connect(control, SIGNAL(volumeChanged(int)), this, SLOT(volumeChange(int)));

    layout->addWidget(labelContainer);
    layout->addWidget(sliderContainer);
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


#include "ControlSlider.moc"
