/*
 * Copyright 2026, Fishpond
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <LayoutBuilder.h>
#include <math.h>
#include "Voice.h"
#include "VoiceControls.h"

VoiceParam::VoiceParam(const char* name, VoiceParamType paramType, VoiceParamDisplay paramDisplay,
        float value, float minValue, float maxValue)
    :
    fName(name),
    fType(paramType),
    fDisplay(paramDisplay)
{
    this->value.floatValue = value;
    fMinValue.floatValue = minValue;
    fMaxValue.floatValue = maxValue;
}

VoiceParam::~VoiceParam() {
    if (fType == GROUP_TYPE && value.paramGroupValue) {
        value.paramGroupValue->MakeEmpty(true);
    }
}

VoiceNote::VoiceNote(Voice* voice, uint8 note, float frequency,
        uint8 velocity, bigtime_t performanceTime)
    :
    fVoice(voice),
    fNote(note),
    fFrequency(frequency),
    fVelocity(velocity / 255.0),
    fState(ACTIVE),
    fNoteOnTime(performanceTime)
{
    SetSampleRate(voice->fSampleRate);
}
float
VoiceNote::SampleRate() {
    return fSampleRate;
}

void VoiceNote::SetSampleRate(float sampleRate) {
    fSampleRate = sampleRate;
    fdT         = 1.0 / sampleRate;
}

void VoiceNote::NoteOff(bigtime_t eventTime) {
    fNoteOffTime = eventTime;
    fState = RELEASE;
}

void VoiceNote::Recycle() {
    fVoice->Recycle(this);
}

VoiceNote::~VoiceNote() { }

Voice::Voice(float sampleRate)
    :
    BArchivable(),
    fSampleRate(sampleRate),
    fVoiceNotes()
{ }

Voice::Voice(BMessage* archive)
    :
    BArchivable(archive),
    fSampleRate(0.0F),
    fVoiceNotes()
{ }

Voice::~Voice() {
    fVoiceNotes.MakeEmpty();
    fParams.MakeEmpty();
}

status_t
Voice::Archive(BMessage* into, bool deep) const {
    return BArchivable::Archive(into, deep);
}

VoiceNote*
Voice::NoteOn(uint8 note, float frequency, uint8 velocity, bigtime_t performanceTime) {
    VoiceNote* voiceNote = createNote(note, frequency, velocity, performanceTime);
    fVoiceNotes.AddItem(voiceNote);
    return voiceNote;
}

bool
Voice::NoteOff(uint8 note, bigtime_t performanceTime) {
    for (int n = 0; n < fVoiceNotes.CountItems(); n++) {
        VoiceNote* voiceNote = fVoiceNotes.ItemAt(n);
        if (voiceNote->fNote == note && voiceNote->fState == VoiceNote::voiceState::ACTIVE) {
            voiceNote->NoteOff(performanceTime);
            return true;
        }
    }
    return false;
}

void
Voice::Recycle(VoiceNote* voiceNote) {
    fVoiceNotes.RemoveItem(voiceNote, true);
}

void
Voice::SetSampleRate(float sampleRate) {
    fSampleRate = sampleRate;
}

BObjectList<VoiceNote, true>*
Voice::ActiveNotes() {
    return &fVoiceNotes;
}

BView*
Voice::CreateParamView() {

    BLayoutBuilder::Group<> group = BLayoutBuilder::Group(B_HORIZONTAL);
    for (int i = 0; i < fParams.CountItems(); i++) {
        VoiceParam* param = fParams.ItemAt(i);

        BString format;
        int places = trunc(std::log10(param->MaxValue().floatValue) + 0.7F);
        if (places < 5)
            format.SetToFormat("%%.%df", 5 - places);
        else
            format.SetTo("%.0f");

        BString labelText;
        labelText.SetToFormat(format, param->value.floatValue);
        BStringView* label = new BStringView(param->Name(), labelText, B_WILL_DRAW | B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT );
        label->SetExplicitAlignment(BAlignment(B_ALIGN_CENTER, B_ALIGN_TOP));
        switch (param->Display()) {
            case VoiceParam::DISPLAY_SLIDER: {
                group.AddGroup(B_VERTICAL)
                    .Add(label)
                    .Add(new SliderFloat(param, label, strdup(format)))
                .End();
                break;
            }
            case VoiceParam::DISPLAY_KNOB: {
                group.AddGroup(B_VERTICAL)
                    .Add(label)
                    .Add(new KnobFloat(param, label, strdup(format)))
                .End();
                break;
            }


        }
    }
    return group.View();
}
