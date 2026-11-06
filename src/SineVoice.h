/*
 * Copyright 2026, YourName
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SINENOTE_H
#define SINENOTE_H


#include <SupportDefs.h>
#include "Voice.h"

class SineVoice;

class SineVoiceNote : public VoiceNote {
public:
                                SineVoiceNote(SineVoice* voice, uint8 note, float frequency,
                                        uint8 velocity, bigtime_t performanceTime);
    virtual float               NextSample() override;
private:
    float                       fPhase;
    float                       fdPhase;
    float                       fModPhase;
    VoiceParam*                 fModPhaseParam;
    VoiceParam*                 fModInfluenceParam;
    float                       fRelease;
};


class SineVoice : public Voice {
    friend class                SineVoiceNote;
public:
                                SineVoice(float sampleRate);
                                SineVoice(BMessage* archive);
    virtual status_t            Archive(BMessage* into, bool deep = true) const override;
    static BArchivable*         Instantiate(BMessage* archive);
protected:
    virtual VoiceNote*          createNote(uint8 note, float frequency, uint8 velocity, bigtime_t performanceTime) override;
    virtual const char*         Name() override { return "Modulated Sine"; }

    VoiceParam*                 fModPhaseParam;
    VoiceParam*                 fModInfluenceParam;
};
#endif // SINENOTE_H
