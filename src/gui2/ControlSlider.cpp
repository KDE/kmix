#include "ControlSlider.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSlider>

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
    qDebug() << control->channels() << "channels";
    for(int i = 0;i<2/*control->channels()*/;i++) {
        QSlider *slider = new QSlider(sliderContainer);
        sliderLayout->addWidget(slider);
        slider->setMaximum(65536);
        slider->setValue(control->getVolume(i));
        m_sliders << slider;
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

#include "ControlSlider.moc"
