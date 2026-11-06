/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SYNTH_H
#define SYNTH_H


#include <SupportDefs.h>
#include <MidiDefs.h>
#include <MidiConsumer.h>
#include <MediaRoster.h>
#include <Messenger.h>
#include "SynthOut.h"
#include "Voice.h"

#define MAX_VOICES 128

class Synth : public BMidiLocalConsumer {
public:
							Synth(BMessenger* notify = NULL);
	virtual                 ~Synth();

    struct                  Channel {
                                Voice* voice;
                            };

	//  Midi Hook Functions
	bigtime_t 				Latency() const;
	void 					SetLatency(bigtime_t latency);
	int32 					GetProducerID();
    media_raw_audio_format* OutputFormat() { return &fOutputFormat; }

	void 					SetTimeout(bigtime_t when, void *data);
	virtual void 			Timeout(void *data);
	virtual void 			Data(uchar *data, size_t length, bool atomic, bigtime_t time);

    float                   Frequency(uint8 note);  // Helper for midi note frequency
    status_t                Start();
    status_t                Stop();
    int                     NextFrame(float* frame);
    void                    SetPan(BPoint p); // Assuming a room of 1.0 x 1.0 units
    Voice*                  GetVoice(int channel);
    void                    SetVoice(Voice* voice, int channel);
protected:
    status_t                InitOutput();
    status_t                InitVoices(float frameRate);

    BObjectList<Voice, true>
                            fVoices;
    Voice*                  fChannels[16];

private:
	BMessenger*				fNotify;
	BMediaRoster*			fRoster;
	media_node				fMixer;
    media_output            fOutput;
    media_input             fInput;
	SynthOut*				fOut;
    media_raw_audio_format  fOutputFormat;
    float                   fPan[32];
    status_t                fStatus;
	const int32				kProducerId = 'pBNG';

    static void             InitFrequencies();
    static float*           kNoteFrequencies;
};

#endif // SYNTH_H
