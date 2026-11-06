/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdlib.h>
#include <stdio.h>
#include <TimeSource.h>
#include <limits.h>
#include "Debug.h"
#include "Synth.h"
#include "SynthOut.h"
#include "SineVoice.h"


Synth::Synth(BMessenger* notify)
	:
	BMidiLocalConsumer("bingbing"),
	fNotify(notify),
	fRoster(NULL),
    fVoices()
{
    if ((fStatus = InitVoices(48000.0)) != B_OK)
        return;

    Voice* voice = fVoices.FirstItem();
    voice->NoteOn(69, Frequency(69), 100, 0);
    voice->NoteOn(72, Frequency(72), 100, 0);
    voice->NoteOn(76, Frequency(76), 100, 0);

    if ((fStatus = InitOutput()) != B_OK)
        return;

    if ((fStatus = InitVoices(fOut->OutputFormat()->u.raw_audio.frame_rate)) != B_OK)
        return;

    Start();
}

Synth::~Synth()
{
    Stop();

    fRoster->UnregisterNode(fOut);
    fVoices.MakeEmpty();
	// delete fNotify;
}


const char* notes[] = {
	"C%d", "C#%d",
	"D%d", "D#%d",
	"E%d",
	"F%d", "F#%d",
	"G%d", "G#%d",
	"A%d", "A#%d",
	"H%d"
};

static char buffer[5] = "    ";

const char* Note(uint8 note) {
	div_t octaveNote = div(note, 12);
	const char* name = notes[octaveNote.rem];
	sprintf(buffer, name, octaveNote.quot - 1);
	return buffer;
}

status_t
Synth::InitOutput() {
    status_t status = B_ERROR;

    fOut = new SynthOut(this);

	fRoster = BMediaRoster::Roster(&fStatus);
    if (!fRoster) {
        TRACE("Synth::Synth: No BMediaRoster obtained\n");
        return status;
    }

	fStatus = fRoster->RegisterNode(fOut);
    if (fStatus != B_OK) {
        TRACE("Synth::Synth: Failed to register SynthOut - %s\n", strerror(fStatus));
        return status;
    } else
        TRACE("Synth::Synth: SynthOut registered.\n");


	fStatus = fRoster->GetAudioMixer(&fMixer);
    if (fStatus != B_OK) {
        TRACE("Synth::Synth: Failed to get mixer - %s\n", strerror(fStatus));
        return status;
    } else
        TRACE("Synth::Synth: Mixer received.%s\n");

    media_output synthOutput;
    int32 cookie = 0;
    fStatus = fOut->GetNextOutput(&cookie, &synthOutput);
    if (fStatus != B_OK) {
        TRACE("Synth::Synth: Failed to get SynthOut output - %s\n", strerror(fStatus));
        return status;
    } else
        TRACE("Synth::Synth: Got SynthOut output (node=%d, port=%d, name=%s)\n",
            synthOutput.node.node,
            synthOutput.source.port,
            synthOutput.name);

    fOut->DisposeOutputCookie(cookie);

    media_input mixerInput;
    cookie = 0;
    int32 found;
    status = fRoster->GetFreeInputsFor(fMixer, &mixerInput, 1, &found, B_MEDIA_RAW_AUDIO);
    if (status != B_OK) {
        TRACE("Synth::Synth: Failed to get Mixer input - %s\n", strerror(fStatus));
        return status;
    } else
        TRACE("Synth::Synth: Got Mixer input (node=%d, port=%d)\n",
            mixerInput.node.node,
            mixerInput.destination.port);

    media_format synthFormat = synthOutput.format;
    fOutputFormat = synthOutput.format.u.raw_audio;
    status = fRoster->Connect(synthOutput.source, mixerInput.destination, &synthFormat, &fOutput, &fInput);
    if (status != B_OK) {
        TRACE("Synth::Synth: Failed to connect SyntOut to Mixer - %s\n", strerror(status));
        return status;
    } else {
        fOutputFormat = fOut->OutputFormat()->u.raw_audio;
        SetPan(BPoint(0.5, 0.5));
        TRACE("Synth::Synth: Connect to mixer successful\n");
    }
    return B_OK;
}

status_t
Synth::InitVoices(float sampleRate) {
    fVoices.MakeEmpty();
    Voice* defaultVoice = new SineVoice(sampleRate);
    fVoices.AddItem(defaultVoice);
    for (int i = 0; i <= 16; i++)
        fChannels[i] = defaultVoice;
    return B_OK;
}

Voice*
Synth::GetVoice(int channel) {
    return fChannels[channel];
}

status_t
Synth::Start() {
    if (fStatus != B_OK) return fStatus;

    fRoster->SetRunModeNode(fOut->Node(), BMediaNode::run_mode::B_INCREASE_LATENCY);
    fStatus = fRoster->StartNode(fOut->Node(), 0);

    if (fStatus != B_OK) {
        TRACE("Synth::Synth: Failed to start SynthOut - %s\n", strerror(fStatus));
    } else
        TRACE("Synth::Synth: Successfully started SynthOut\n");
    return fStatus;
}

status_t
Synth::Stop() {
    fRoster->StopNode(fOut->Node(), 0, false);
    if (fOutput.source != media_source::null && fInput.destination != media_destination::null)
        fRoster->Disconnect(fOutput.node.node, fOutput.source, fInput.node.node, fInput.destination);

    return fStatus;
}

int
Synth::NextFrame(float* frame) {
    float value = 0.0;
    for (int v = 0; v < fVoices.CountItems(); v++) {
        Voice* voice = fVoices.ItemAt(v);
        BObjectList<VoiceNote, true>* notes = voice->ActiveNotes();
        for (int n = 0; n < notes->CountItems(); n++) {
            VoiceNote* note = notes->ItemAt(n);
            value += note->NextSample();
        }
    }
    for (int c = 0; c < fOutputFormat.channel_count; c++)
        *(frame++) = value * fPan[c];

    return fOutputFormat.channel_count;
}


float dist(BPoint a, BPoint b) {
    a -= b;
    return sqrt(a.x*a.x + a.y*a.y);
}

void
Synth::SetPan(BPoint p) {
    switch (fOutputFormat.channel_count)  {
        case 2:
            fPan[0] = 0.5 + (1.0 - p.x) * 0.5;  // left
            fPan[1] = 0.5 + p.x * 0.5;          // right
            TRACE("Synth:SetPan: Set (%.0f / %.0f) to left=%.0f%% right=%.0f%%\n",
                    p.x * 100 - 50, p.y * 100 - 50,
                    fPan[0] * 100, fPan[1] * 100);
            break;
        case 6:
            fPan[0] = 0.2 + (1.0 - dist(BPoint(0.0, 0.0), p)) * 0.8;  // left
            fPan[1] = 0.2 + (1.0 - dist(BPoint(1.0, 0.0), p)) * 0.8;  // right
            fPan[2] = 0.2 + (1.0 - dist(BPoint(0.5, 0.5), p)) * 0.8;  // center
            fPan[3] = 1.0;                                            // sub
            fPan[4] = 0.2 + (1.0 - dist(BPoint(0.0, 1.0), p)) * 0.8;  // rear left
            fPan[5] = 0.2 + (1.0 - dist(BPoint(1.0, 1.0), p)) * 0.8;  // rear right
            TRACE("Synth:SetPan: Set (%.0f / %.0f) to left=%.0f%% right=%.0f%% center=%.0f%% sub=%.0f%% rleft=%.0f%% rright=%.0f%%\n",
                    p.x * 100, p.y * 100,
                    fPan[0] * 100, fPan[1] * 100, fPan[2] * 100, fPan[3] * 100, fPan[4] * 100, fPan[5] * 100);
            break;
        default:
            for (int c = 0; c < fOutputFormat.channel_count; c++)
                fPan[c] = 1.0;
    }

}

void
Synth::Data(uchar *data, size_t length, bool atomic, bigtime_t time)
{
	uint8 channel = data[0] & 0x0f;
	uint8 command = data[0] & 0xf0;
	switch (command) {
		case B_NOTE_ON: {
			TRACE("Note On:           %s chan: %2d vel: %3d\n", Note(data[1]), channel, data[2]);

            Voice* voice = fVoices.FirstItem();
            voice->NoteOn(data[1], Frequency(data[1]), data[2], fOut->TimeSource()->Now());
			break;
        }
		case B_NOTE_OFF: {
			TRACE("Note Off:          %s chan: %2d vel: %3d\n", Note(data[1]), channel, data[2]);
            Voice* voice = fVoices.FirstItem();
            if (!voice->NoteOff(data[1], fOut->TimeSource()->Now()))
                TRACE("Note Off:          No active note %s found\n", Note(data[1]));
			break;
        }
		case B_KEY_PRESSURE:
			TRACE("Key Pressure:      %s chan: %2d pressure: %3d\n", Note(data[1]), channel, data[2]);
			break;
		case B_CONTROL_CHANGE:
			TRACE("Control Change:   %3d chan: %2d value: %3d\n", data[1], channel, data[2]);
			break;
		case B_PROGRAM_CHANGE:
			TRACE("Program Change:   %3d chan: %2d\n", data[1], channel);
			break;
		case B_CHANNEL_PRESSURE:
			TRACE("Channel Pressure: %3d chan: %2d\n", data[1], channel);
			break;
		case B_PITCH_BEND: {
			int16 value = data[1] + (data[2] >> 8);
			TRACE("Channel Pressure: %5d chan: %2d\n", value, channel);
			break;
		}
		default:
			TRACE("Data (%d): ", length);
			for (int i = 0; i < length; i++)
				TRACE("%02x ", data[i]);
			TRACE("\n");
			break;
	}
}

float Synth::Frequency(uint8 note) {
    if (kNoteFrequencies == NULL)
        InitFrequencies();
    return kNoteFrequencies[note];
}

void Synth::Timeout(void *data) {
    TRACE("Synth:Timeout\n");
}


float* Synth::kNoteFrequencies = NULL;

void Synth::InitFrequencies() {
    kNoteFrequencies = new float[128];
    for (int note = 0; note < 128; note++) {
        kNoteFrequencies[note] = pow(2.0, (note - 69.0) / 12.0) * 440.0;
        #ifdef DEBUGGING
        TRACE("%s: %4.2f\n", Note(note), kNoteFrequencies[note]);
        #endif
    }
}
