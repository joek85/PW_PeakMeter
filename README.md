# PW_PeakMeter
Audio PeakMeter for JackAudio
<br>
<img height="180" src="https://github.com/joek85/PW_PeakMeter/blob/master/PW_PeakMeter_image.png?raw=true" />
<br>

Current values:
*Meter scale:-40db - +3db
*Mode:Peak
*Decay speed:customizable
*Hold time:customizable

This project aims to display audio peak levels using jackaudio as source for audio data flow from your soundcard.
By default audio input of the meter is automaticaly connected to the first audio input source.
GTK2 is used for window creation and bitmap,the latter is refreshed by a 15ms gtk timer,lower interval needs more cpu power but 15 is enough.
Bitmap colors are customizable(single color or multi-color).

Future features:
*orientation:horizontal - vertical
*Modes:VU-PPM-RMS
*Captions for scale values
