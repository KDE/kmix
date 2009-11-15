

QString Mixer_Backend::translateKernelToWhatsthis(const QString &kernelName)
{
        if (kernelName == "Mic:0") return i18n("Recording level of the microphone input.");
	else if (kernelName == "Master:0") return i18n("Controls the volume of the front speakers or all speakers (depending on your soundcard model). If you use a digital output, you might need to also use other controls like ADC or DAC. For headphones soundcards often supply a Headphone control.");
	else if (kernelName == "PCM:0") return i18n("Most media like MP3 or Videos are played back using PCM. For playControls the volume of the front speakers or all speakers (depending on your soundcard model). If you use a digital output, you might need to also use other controls like ADC or DAC.");
	else if (kernelName == "Headphone:0") return i18n("Controls the headphone volume. Some soundcards include a switch that must be manually activated to enable the headphone output.");
	else return i18n("---"); 
}