#include "ControlSlider.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSlider>
#include <QtCore/QSignalMapper>

#include "control_interface.h"

ControlSlider::ControlSlider(org::kde::KMix::Control *control, QWidget *parent)
    : QWidget(parent)
    , m_control(control)
{
    QVBoxLayout *layout = new QVBoxLayout();
    QLabel *label = new QLabel(m_control->displayName(), this);
    setLayout(layout);

    QWidget *sliderContainer = new QWidget(this);
    layout->addWidget(label);
    layout->addWidget(sliderContainer);
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
