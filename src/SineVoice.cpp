/*
 * Copyright 2026, YourName
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <math.h>
#include <Message.h>
#include "SineVoice.h"


SineVoice::SineVoice(float sampleRate)
    :
    Voice(sampleRate),
    fModPhaseParam(new VoiceParam("Modulator Rate", VoiceParam::FLOAT_TYPE, VoiceParam::DISPLAY_KNOB, 3.5F, 0.1F, 20.0F)),
    fModInfluenceParam(new VoiceParam("Modulator Influence", VoiceParam::FLOAT_TYPE, VoiceParam::DISPLAY_KNOB, 0.3F, 0.0F, 2.0F))
{
    fParams.AddItem(fModPhaseParam);
    fParams.AddItem(fModInfluenceParam);
}

SineVoice::SineVoice(BMessage* archive)
    :
    Voice(archive),
    fModPhaseParam(new VoiceParam("Modulator Rate", VoiceParam::FLOAT_TYPE, VoiceParam::DISPLAY_KNOB, 3.5F, 0.1F, 20.0F)),
    fModInfluenceParam(new VoiceParam("Modulator Influence", VoiceParam::FLOAT_TYPE, VoiceParam::DISPLAY_KNOB, 0.3F, 0.0F, 2.0F))
{
    fParams.AddItem(fModPhaseParam);
    fParams.AddItem(fModInfluenceParam);

    archive->FindFloat("sine:modulator_rate", &fModPhaseParam->value.floatValue);
    archive->FindFloat("sine:modulator_influence", &fModInfluenceParam->value.floatValue);
}

status_t
SineVoice::Archive(BMessage* into, bool deep) const {
    status_t status = Voice::Archive(into, deep);
    if (status != B_OK)
        return status;

    status = into->AddFloat("sine:modulator_rate", fModPhaseParam->value.floatValue);
    if (status != B_OK)
        return status;

    return into->AddFloat("sine:modulator_influence", fModInfluenceParam->value.floatValue);
}

BArchivable*
SineVoice::Instantiate(BMessage* archive) {
    if (!validate_instantiation(archive, "SineVoice"))
        return NULL;

    return new SineVoice(archive);
}

VoiceNote*
SineVoice::createNote(uint8 note, float frequency, uint8 velocity, bigtime_t performanceTime) {
    return new SineVoiceNote(this, note, frequency, velocity, performanceTime);
}

SineVoiceNote::SineVoiceNote(SineVoice* voice, uint8 note, float frequency, uint8 velocity, bigtime_t eventTime)
    :
    VoiceNote(voice, note, frequency, velocity, eventTime),
    fPhase(0.0),
    fdPhase(2.0 * M_PI * fdT * fFrequency),
    fModPhase(0.0),
    fModPhaseParam(voice->fModPhaseParam),
    fModInfluenceParam(voice->fModInfluenceParam),
    fRelease(1.0/20000.0)
{ }

float
SineVoiceNote::NextSample() {
    fModPhase += fdPhase * fModPhaseParam->value.floatValue;
    fPhase += fdPhase * (1.0 + fModInfluenceParam->value.floatValue * sin(fModPhase));
    while (fPhase > 2.0 * M_PI)
        fPhase -= 2.0 * M_PI;
    while (fModPhase > 2.0 * M_PI)
        fModPhase -= 2.0 * M_PI;

    if (fState == RELEASE) {
        fVelocity -= fRelease;
        if (fVelocity <= 0.0)
            Recycle();
    }

    return sin(fPhase) * fVelocity;
}
