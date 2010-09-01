/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2006-2009 Christian Esken <esken@kde.org>
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


QString Mixer_Backend::translateKernelToWhatsthis(const QString &kernelName)
{
        if (kernelName == "Mic:0") return i18n("Recording level of the microphone input.");
	else if (kernelName == "Master:0") return i18n("Controls the volume of the front speakers or all speakers (depending on your soundcard model). If you use a digital output, you might need to also use other controls like ADC or DAC. For headphones, soundcards often supply a Headphone control.");
	else if (kernelName == "PCM:0") return i18n("Most media, such as MP3s or Videos, are played back using the PCM channel. As such, the playback volume of such media is controlled by both this and the Master or Headphone channels.");
	else if (kernelName == "Headphone:0") return i18n("Controls the headphone volume. Some soundcards include a switch that must be manually activated to enable the headphone output.");
	else return i18n("---"); 
}
