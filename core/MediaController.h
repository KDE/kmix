/*
 * MediaController.h
 *
 *  Created on: 17.12.2013
 *      Author: chris
 */

#ifndef MEDIACONTROLLER_H_
#define MEDIACONTROLLER_H_

#include <QString>

/**
 * A MediaController controls exactly one Media Player. You can think of it as a single control, like PCM.
 */
class MediaController
{
public:
    enum PlayState { PlayPaused, PlayPlaying, PlayStopped, PlayUnknown };

	MediaController(QString);
	virtual ~MediaController();

	   void addMediaPlayControl() { mediaPlayControl = true; };
	   void addMediaNextControl() { mediaNextControl = true; };
	   void addMediaPrevControl() { mediaPrevControl = true; };
	   bool hasMediaPlayControl() { return mediaPlayControl; };
	   bool hasMediaNextControl() { return mediaNextControl; };
	   bool hasMediaPrevControl() { return mediaPrevControl; };
	    bool hasControls();


	MediaController::PlayState getPlayState();
    void setPlayState(PlayState playState);

    bool canSkipNext();
    bool canSkipPrevious();

private:
    QString id;
    PlayState playState;

    bool mediaPlayControl;
    bool mediaNextControl;
    bool mediaPrevControl;
};

#endif /* MEDIACONTROLLER_H_ */
