/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <MediaDefs.h>
#include <MediaRoster.h>
#include <TimedEventQueue.h>
#include <Buffer.h>
#include <TimeSource.h>
#include "Debug.h"
#include "SynthOut.h"
#include "Synth.h"


const char* SynthOut::kName 		= "bingbing";
const char* SynthOut::kNameStereo 	= "bingbing stereo";

SynthOut::SynthOut(Synth* synth)
	:
	BMediaNode(kName),
	BMediaEventLooper(),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	fSynth(synth),
	fFrameRate(48000),
	fBufferDuration(20000),
    fBufferCount(2),
	fLatency(20000 * 2),
    fOutputEnabled(true),
    fBufferGroup(NULL),
    fdT(1.0/48000.0),
	fStatus(B_OK)
{
	fFramesPerBuffer = (fFrameRate * fBufferDuration) / 1000000;

	fPreferredFormat.Clear();
	fPreferredFormat.type = B_MEDIA_RAW_AUDIO;
	fPreferredFormat.u.raw_audio.channel_count = 2;
	fPreferredFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	fPreferredFormat.u.raw_audio.frame_rate = media_raw_audio_format::wildcard.frame_rate;;
	fPreferredFormat.u.raw_audio.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
	fPreferredFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
    fPreferredFormat.u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size;

	/*
    fPreferredFormat.u.raw_audio.channel_count = 6;
	fPreferredFormat.u.raw_audio.channel_mask |= B_CHANNEL_CENTER | B_CHANNEL_SUB |
                                                 B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT;
    */
    fOutput.format = fPreferredFormat;
    fOutput.destination = media_destination::null;
    fOutput.source = media_source::null;
    strcpy(fOutput.name, kNameStereo);

    TRACE("SynthOut::SynthOut: Initialized.\n");
}

SynthOut::~SynthOut() {
	BMediaEventLooper::Quit();
	delete fBufferGroup;
}

const media_format*
SynthOut::OutputFormat() {
    return &fOutput.format;
}


BMediaAddOn*
SynthOut::AddOn(int32* internalID) const {
	return NULL;
}

status_t
SynthOut::GetLatency(bigtime_t* latency) {
    *latency= fLatency;
    return B_OK;
}


void
SynthOut::NodeRegistered() {
    TRACE("SynthOut::NodeRegistered: Starting looper...\n");

    fOutput.node = Node();
    fOutput.source.port = ControlPort();
    fOutput.source.id = 0;

	BMediaEventLooper::Run();
}

void
SynthOut::HandleEvent(const media_timed_event* event, bigtime_t lateNess, bool realTimeEvent) {
	switch (event->type) {
		case BTimedEventQueue::B_START:
			// Start your data-generation loops or timers here
			if (RunState() != BMediaEventLooper::B_STARTED) {
                fStartTime = event->event_time;
                fFramesSent = 0;
				media_timed_event startBufferEvent(fStartTime, BTimedEventQueue::B_HANDLE_BUFFER);
				EventQueue()->AddEvent(startBufferEvent);
			}
			break;

		case BTimedEventQueue::B_HANDLE_BUFFER:
			// Nur Buffer senden, wenn der Node aktiv läuft und verbunden ist
			if (RunState() == BMediaEventLooper::B_STARTED &&
                fOutput.destination != media_destination::null )
            {
                status_t status = B_ERROR;
				BBuffer* buffer = FillBuffer(event->event_time);
                if (buffer) {
                    if (fOutputEnabled) {
                        status = SendBuffer(buffer, fOutput.source, fOutput.destination);
                        //TRACE("Buffer sent with status %d\n", status);
                    }

                    if (status)
                        buffer->Recycle();
                }
                fFramesSent += fFramesPerBuffer;

				// Nächstes Buffer-Event für die Zukunft einplanen
				media_timed_event nextEvent(fStartTime + fFramesSent / fFrameRate * 1000000, BTimedEventQueue::B_HANDLE_BUFFER);
				EventQueue()->AddEvent(nextEvent);
			}
			break;

		case BTimedEventQueue::B_STOP:
			// Stop generating data
			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
 			break;

		default:
			break;
	}
}

status_t SynthOut::FormatSuggestionRequested(media_type type, int32 quality, media_format* format){
	*format = fPreferredFormat;
	return B_OK;
}

status_t SynthOut::FormatProposal(const media_source& output, media_format* ioFormat) {
    if (ioFormat->type != B_MEDIA_RAW_AUDIO)
        return B_ERROR;
    *ioFormat = fPreferredFormat;
	return B_OK;
}

status_t SynthOut::GetNextOutput(int32* cookie, media_output* outOutput) {
    TRACE("SynthOut::GetNextOutput: Requested next output (cookie=%d)\n", *cookie);
	if (*cookie != 0) return B_BAD_INDEX;

    *outOutput = fOutput;
    strcpy(outOutput->name, kNameStereo);
    (*cookie)++;
    return B_OK;
}


status_t SynthOut::DisposeOutputCookie(int32 cookie) {
    return B_OK;
}


status_t SynthOut::PrepareToConnect(const media_source& source, const media_destination& destination,
								  media_format* format, media_source* out_source, char* out_name)
{
    TRACE("SynthOut::PrepareToConnect: connection from port=%d, id=%d to port=%d, id=%d\n",
        source.port, source.id, destination.port, destination.id);
    TRACE("                            with format chan=%d, sr=%f, size=%d\n",
        format->u.raw_audio.channel_count, format->u.raw_audio.frame_rate,
        format->u.raw_audio.buffer_size);

	if (source.id != 0)
		return B_BAD_INDEX;
	if (format->type != B_MEDIA_RAW_AUDIO)
		return B_ERROR;

    fBufferDuration = 20000;
    format->u.raw_audio.buffer_size = fBufferDuration * format->u.raw_audio.frame_rate / 1000000 * format->AudioFrameSize();
    format->u.raw_audio.channel_count = fPreferredFormat.u.raw_audio.channel_count;
    format->u.raw_audio.channel_mask = fPreferredFormat.u.raw_audio.channel_mask;

    *out_source = source;
    strcpy(out_name, kNameStereo);

	return B_OK;
}

void SynthOut::Connect(status_t error, const media_source& source, const media_destination& destination,
                         const media_format& format, char* io_name)
{
	if (error != B_OK) return;

    TRACE("SynthOut::Connect: connection from port=%d, id=%d to port=%d, id=%d\n",
        source.port, source.id, destination.port, destination.id);
    TRACE("                            with format chan=%d, frameRate=%f, buffer=%d\n",
        format.u.raw_audio.channel_count, format.u.raw_audio.frame_rate,
        format.u.raw_audio.buffer_size);


    fOutput.source = source;
    fOutput.destination = destination;
    fOutput.format = format;

    fFrameRate = format.u.raw_audio.frame_rate;
    if (format.u.raw_audio.buffer_size == 0) {
        fBufferDuration = 20000;
        fFramesPerBuffer = fBufferDuration * fFrameRate / 1000000;
        fOutput.format.u.raw_audio.buffer_size = fFramesPerBuffer * format.AudioFrameSize();
    } else {
        fFramesPerBuffer = fOutput.format.u.raw_audio.buffer_size / format.AudioFrameSize();
        fBufferDuration = fFramesPerBuffer * 1000000 / fFrameRate;
    }

    TRACE("SynthOut::Connect: buffer=%ld, frameRate=%ld, chan: %d -> fpb=%ld, dur=%ld)\n",
        fOutput.format.u.raw_audio.buffer_size, fFrameRate, fOutput.format.u.raw_audio.channel_count,
        fFramesPerBuffer, fBufferDuration);

    SetBufferDuration(fBufferDuration);
    fBufferGroup = new BBufferGroup(fOutput.format.u.raw_audio.buffer_size, fBufferCount);

    media_node_id id;
	FindLatencyFor(fOutput.destination, &fLatency, &id);


    // Use a dry run to see how long it takes me to fill a buffer of data
	bigtime_t start, internalLatency;
	float* data = new float[fFramesPerBuffer];
    fdT    = 1.0 / fFrameRate;
	start = ::system_time();
    BBuffer* buffer = FillBuffer(start);
	internalLatency = ::system_time() - start;
    buffer->Recycle();

	TRACE("SynthOut::Connect: Downstream latency=%" B_PRIdBIGTIME ", internal latency=%" B_PRIdBIGTIME "\n",
            fLatency, internalLatency);

    SetEventLatency(fLatency + internalLatency * 3.0);
    strcpy(fOutput.name, io_name);
}

void SynthOut::Disconnect(const media_source& source, const media_destination& destination)
{
	if (source.id != 0 || fOutput.destination != destination) return;

    TRACE("SynthOut::Disconnect: Disconnect requested for %s\n", source.id);
	fOutput.destination = media_destination::null;
	delete fBufferGroup;
	fBufferGroup = nullptr;
}

status_t SynthOut::SetBufferGroup(const media_source& forSource, BBufferGroup* group) {
    delete fBufferGroup;
	fBufferGroup = group;
    int32 count;
    fBufferGroup->CountBuffers(&count);
    TRACE("SynthOut::SetBufferGroup: Buffergroup received (count=%d)\n", count);
	return B_OK;
}

BBuffer* SynthOut::FillBuffer(bigtime_t performanceTime)
{
	// Fordere einen freien Speicherbereich aus der Gruppe an
    BBuffer* buffer = fBufferGroup->RequestBuffer(fOutput.format.u.raw_audio.buffer_size, fBufferDuration);
    // System ist überlastet oder blockiert
    if (!buffer)
        return buffer;

    int32 bufferSize = buffer->Size();
    int32 frames = bufferSize / fOutput.format.AudioFrameSize();
	float* data = (float*)buffer ->Data();
	float fFrequency = 440;
	// Mathematische Generierung der Sinuswelle (Stereo)
    float fPhaseDelta = 2.0 * M_PI * fFrequency * fdT;
    //TRACE("SynthOut::FillBuffer: Time=%f, Period=%f, Frames=%df\n",
    //    performanceTime / 1000000.0, fPhaseDelta * frames, frames);
	for (uint32 i = 0; i < frames; i++) {
        data += fSynth->NextFrame(data);
	}

	// Metadaten für das Media-Kit setzen
	media_header* header = buffer->Header();
	header->type = B_MEDIA_RAW_AUDIO;
	header->size_used = bufferSize;
	header->time_source = this->TimeSource()->ID();

    if (RunMode() == B_RECORDING)
        header->start_time = performanceTime; // Exakte Wiedergabezeit
    else
        header->start_time = fStartTime + (bigtime_t)((double)fFramesSent / (double)fOutput.format.u.raw_audio.frame_rate * 1000000.0);
    return buffer;
}

void SynthOut::EnableOutput(const media_source& what, bool enabled, int32* _deprecated_) {
    if (what.id == fOutput.source.id && what.port == fOutput.source.port) {
        TRACE("SynthOut::EnableOutput: Output %s\n", enabled ? "enabled" : "disabled");
        fOutputEnabled = enabled;
    }
}

void SynthOut::LateNoticeReceived(const media_source& what, bigtime_t howMuch, bigtime_t performanceTime) {
    TRACE("SynthOut::LateNoticeReceived: %.4f late\n", howMuch / 1000000.0);
    if (fOutputEnabled) {
        BBuffer* buffer = FillBuffer(performanceTime);
        if (buffer) {
            status_t status = SendBuffer(buffer, fOutput.source, fOutput.destination);
            TRACE("Extra buffer sent with status %d\n", status);
            if (status != B_OK)
                buffer->Recycle();

        }
    }
    fFramesSent += fFramesPerBuffer;
}



status_t SynthOut::FormatChangeRequested(const media_source& source, const media_destination& destination, media_format* ioFormat, int32* _deprecated_) { return B_ERROR; }
status_t SynthOut::VideoClippingChanged(const media_source& forSource, int16 numShorts, int16* clipData, const media_video_display_info& display, int32 * _deprecated_) { return B_ERROR; }
void SynthOut::LatencyChanged(const media_source& source, const media_destination& destination, bigtime_t newLatency, uint32 flags) { }
status_t SynthOut::SetPlayRate(int32 numer, int32 denom) { return B_OK; }



status_t
SynthOut::HandleMessage(int32 message, const void* data, size_t size) {
	switch (message) {
	}
	return B_OK;
}

void
SynthOut::AdditionalBufferRequested(const media_source& source, media_buffer_id previousBuffer,
									bigtime_t previousTime, const media_seek_tag* previousTag)
{

}


