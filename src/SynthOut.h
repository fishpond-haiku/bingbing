/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SYNTHOUT_H
#define SYNTHOUT_H

#include <SupportDefs.h>
#include <BufferProducer.h>
#include <MediaNode.h>
#include <MediaEventLooper.h>
#include <BufferGroup.h>


class Synth;

class SynthOut : public BMediaEventLooper, public BBufferProducer {
public:
								SynthOut(Synth* synth);

    // Management Functions
    const media_format*         OutputFormat();

	// BMediaAddOn hook functions
	virtual BMediaAddOn* 		AddOn(int32* internalID) const override;
    virtual	status_t			GetLatency(bigtime_t* _latency) override;

protected:
	virtual						~SynthOut(); // Called from BMediaNode::Release

	// BMediaEventLooper hook functions
	virtual void				NodeRegistered() override;
	virtual void				HandleEvent(const media_timed_event*, bigtime_t, bool) override;

	// BBufferProducer hook functions
public:
	virtual	status_t			FormatSuggestionRequested(media_type type,
									int32 quality, media_format* format) override;

	virtual	status_t			FormatProposal(const media_source& output,
									media_format* ioFormat) override;

	// If the format isn't good, put a good format into ioFormat and
	// return error.
	// If format has wildcard, specialize to what you can do (and change).
	// If you can change the format, return OK.
	// The request comes from your destination sychronously, so you cannot ask
	// it whether it likes it -- you should assume it will since it asked.
	virtual	status_t			FormatChangeRequested(
									const media_source& source,
									const media_destination& destination,
									media_format* ioFormat,
									int32* _deprecated_) override;

	virtual	status_t			GetNextOutput(
									int32* ioCookie,
									media_output* _output) override;

	virtual	status_t			DisposeOutputCookie(int32 cookie) override;

protected:
	// In this function, you should either pass on the group to your upstream
	// guy, or delete your current group and hang on to this group. Deleting
	// the previous group (unless you passed it on with the reclaim flag set
	// to false) is very important, else you will 1) leak memory and 2) block
	// someone who may want to reclaim the buffers living in that group.
	virtual	status_t			SetBufferGroup(const media_source& forSource,
									BBufferGroup* group) override;

	// Format of clipping is (as int16-s): <from line> <npairs> <startclip>
	// <endclip>. Repeat for each line where the clipping is different from
	// the previous line. If <npairs> is negative, use the data from line
	// -<npairs> (there are 0 pairs after a negative <npairs>. Yes, we only
	// support 32k*32k frame buffers for clipping. Any non-0 field of
	// 'display' means that that field changed, and if you don't support that
	// change, you should return an error and ignore the request. Note that
	// the buffer offset values do not have wildcards; 0 (or -1, or whatever)
	// are real values and must be adhered to.
	virtual	status_t			VideoClippingChanged(
									const media_source& forSource,
									int16 numShorts,
									int16* clipData,
									const media_video_display_info& display,
									int32 * _deprecated_) override;

	virtual	status_t			PrepareToConnect(const media_source& what,
									const media_destination& where,
									media_format* format,
									media_source* _source,
									char* _name) override;

	virtual	void				Connect(status_t error,
									const media_source& source,
									const media_destination& destination,
									const media_format& format,
									char* ioName) override;

	virtual	void				Disconnect(const media_source& what,
									const media_destination& where) override;

	virtual	void				LateNoticeReceived(const media_source& what,
									bigtime_t howMuch,
									bigtime_t performanceTime) override;

	virtual	void				EnableOutput(const media_source& what,
									bool enabled, int32* _deprecated_) override;

	virtual	status_t			SetPlayRate(int32 numer, int32 denom) override;

	// NOTE: Call this from the thread that listens to the port!
	virtual	status_t			HandleMessage(int32 message, const void* data,
									size_t size) override;

	virtual	void				AdditionalBufferRequested(
									const media_source& source,
									media_buffer_id previousBuffer,
									bigtime_t previousTime,
									const media_seek_tag* previousTag) override;

	virtual	void				LatencyChanged(const media_source& source,
									const media_destination& destination,
									bigtime_t newLatency, uint32 flags) override;


private:
	BBuffer*					FillBuffer(bigtime_t performanceTime);

	status_t					fStatus;
	media_format				fPreferredFormat;
	uint64						fFrameRate;
	uint64						fFramesPerBuffer;
    uint64                      fFramesSent;
	bigtime_t					fLatency;
	bigtime_t					fBufferDuration;
    bigtime_t                   fStartTime;
	Synth*						fSynth;
    //double                      fPhase;
    double                      fdT;
	media_output    			fOutput;
    bool                        fOutputEnabled;
	BBufferGroup*				fBufferGroup;
    int                         fBufferCount;

	static const char* 			kName;
	static const char* 			kNameStereo;
};

#endif // SYNTHOUT_H
