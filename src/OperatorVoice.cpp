/*
 * Copyright 2026, Fishpond
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "OperatorVoice.h"

#include <math.h>
#include <new>
#include <string.h>

namespace {

struct NoteState : public OperatorState { };
struct SineState : public OperatorState { float phase; SineState() : phase(0.0f) { } };
struct EnvelopeState : public OperatorState {
    uint64 releaseFrame;
    float releaseAmplitude;
    bool released;
    bool finished;
    EnvelopeState() : releaseFrame(0), releaseAmplitude(0.0f), released(false), finished(false) { }
};
struct NoiseState : public OperatorState { uint32 random; NoiseState() : random(0x9e3779b9) { } };

uint64 TimeToFrames(uint64 usec, float rate)
{
    if (usec == 0 || rate <= 0.0f)
        return 0;
    double frames = (double)usec * rate / 1000000.0;
    return frames >= (double)UINT64_MAX ? UINT64_MAX : (uint64)frames;
}

OperatorValue NoteFrequency(const Operator*, int32, OperatorVoiceNote& note, OperatorState&)
    { return OperatorValue::Float(note.NoteFrequency()); }
OperatorValue NoteVelocity(const Operator*, int32, OperatorVoiceNote& note, OperatorState&)
    { return OperatorValue::Float(note.NoteVelocity()); }
OperatorValue NoteOffValue(const Operator*, int32, OperatorVoiceNote& note, OperatorState&)
    { return OperatorValue::Event(note.Frame()); }

OperatorValue SineSample(const Operator* op, int32, OperatorVoiceNote& note, OperatorState& base)
{
    SineState& state = static_cast<SineState&>(base);
    float frequency = note.InputValue(op, "Frequency").data.floatValue;
    float amplitude = note.InputValue(op, "Amplitude").data.floatValue;
    if (note.SampleRate() > 0.0f) {
        state.phase += (float)(2.0 * M_PI) * frequency / note.SampleRate();
        if (state.phase >= (float)(2.0 * M_PI))
            state.phase = fmodf(state.phase, (float)(2.0 * M_PI));
    }
    return OperatorValue::Float(sinf(state.phase) * amplitude);
}

OperatorValue EnvelopeAmplitude(const Operator* op, int32, OperatorVoiceNote& note,
    OperatorState& base)
{
    EnvelopeState& state = static_cast<EnvelopeState&>(base);
    if (state.finished)
        return OperatorValue::Float(0.0f);
    uint64 attack = TimeToFrames(note.InputValue(op, "Attack").data.timeValue, note.SampleRate());
    uint64 decay = TimeToFrames(note.InputValue(op, "Decay").data.timeValue, note.SampleRate());
    uint64 release = TimeToFrames(note.InputValue(op, "Release").data.timeValue, note.SampleRate());
    float sustain = note.InputValue(op, "Sustain").data.floatValue;
    uint64 frame = note.Frame();
    if (state.released) {
        uint64 elapsed = frame >= state.releaseFrame ? frame - state.releaseFrame : 0;
        if (release == 0 || elapsed >= release) {
            state.finished = true;
            return OperatorValue::Float(0.0f);
        }
        return OperatorValue::Float(state.releaseAmplitude
            * (1.0f - (float)elapsed / (float)release));
    }
    if (attack > 0 && frame < attack)
        return OperatorValue::Float((float)frame / (float)attack);
    if (decay > 0 && frame - (frame >= attack ? attack : frame) < decay
        && frame >= attack) {
        float progress = (float)(frame - attack) / (float)decay;
        return OperatorValue::Float(1.0f + (sustain - 1.0f) * progress);
    }
    return OperatorValue::Float(sustain);
}

OperatorValue NoiseSample(const Operator*, int32, OperatorVoiceNote&, OperatorState& base)
{
    NoiseState& state = static_cast<NoiseState&>(base);
    state.random ^= state.random << 13;
    state.random ^= state.random >> 17;
    state.random ^= state.random << 5;
    return OperatorValue::Float((float)(int32)state.random / 2147483648.0f);
}

Operator* InstantiateOperator(const char* type, BMessage* archive)
{
    if (strcmp(type, "NoteOperator") == 0) return new(std::nothrow) NoteOperator(archive);
    if (strcmp(type, "SineOperator") == 0) return new(std::nothrow) SineOperator(archive);
    if (strcmp(type, "EnvelopeOperator") == 0) return new(std::nothrow) EnvelopeOperator(archive);
    if (strcmp(type, "NoiseOperator") == 0) return new(std::nothrow) NoiseOperator(archive);
    return NULL;
}

} // namespace

OperatorValue OperatorValue::Float(float value)
    { OperatorValue result; result.type = FLOAT_VALUE; result.data.floatValue = value; return result; }
OperatorValue OperatorValue::Event(uint64 frame)
    { OperatorValue result; result.type = EVENT_VALUE; result.data.timeValue = frame; return result; }
OperatorValue OperatorValue::Bool(bool value)
    { OperatorValue result; result.type = BOOL_VALUE; result.data.boolValue = value; return result; }
OperatorValue OperatorValue::Selector(int32 value)
    { OperatorValue result; result.type = SELECTOR_VALUE; result.data.selectorValue = value; return result; }
OperatorValue OperatorValue::Time(uint64 value)
    { OperatorValue result; result.type = TIME_VALUE; result.data.timeValue = value; return result; }

OperatorInput::OperatorInput(const char* inputName, OperatorValue::Type inputType,
    const OperatorValue& value, const char* const* names, int32 count)
    : name(inputName), type(inputType), defaultValue(value), source(NULL),
      selectorNames(names), selectorNameCount(count) { }

OperatorOutput::OperatorOutput(Operator* operatorOwner, const char* outputName,
    OperatorValue::Type outputType, OperatorOutputFunction outputFunction)
    : owner(operatorOwner), name(outputName), type(outputType),
      function(outputFunction), cacheIndex(-1), eventTargets() { }

Operator::Operator()
    : BArchivable(), fID(-1), fGraphIndex(-1), fPosition(0.0f, 0.0f), fInputs(), fOutputs() { }
Operator::Operator(BMessage* archive)
    : BArchivable(archive), fID(-1), fGraphIndex(-1), fPosition(0.0f, 0.0f), fInputs(), fOutputs() { }
Operator::~Operator() { }
int32 Operator::ID() const { return fID; }
BPoint Operator::Position() const { return fPosition; }
void Operator::SetPosition(BPoint position) { fPosition = position; }
int32 Operator::InputCount() const { return fInputs.CountItems(); }
int32 Operator::OutputCount() const { return fOutputs.CountItems(); }
OperatorInput* Operator::InputAt(int32 index) const { return fInputs.ItemAt(index); }
OperatorOutput* Operator::OutputAt(int32 index) const { return fOutputs.ItemAt(index); }

OperatorInput* Operator::FindInput(const char* name) const
{
    if (name == NULL) return NULL;
    for (int32 i = 0; i < InputCount(); i++)
        if (fInputs.ItemAt(i)->name == name) return fInputs.ItemAt(i);
    return NULL;
}
OperatorOutput* Operator::FindOutput(const char* name) const
{
    if (name == NULL) return NULL;
    for (int32 i = 0; i < OutputCount(); i++)
        if (fOutputs.ItemAt(i)->name == name) return fOutputs.ItemAt(i);
    return NULL;
}
const char* const* Operator::SelectorNames(const char* inputName, int32* count) const
{
    OperatorInput* input = FindInput(inputName);
    if (count != NULL) *count = input != NULL ? input->selectorNameCount : 0;
    return input != NULL ? input->selectorNames : NULL;
}

status_t Operator::AddInput(const char* name, OperatorValue::Type type,
    const OperatorValue& value, const char* const* selectorNames, int32 count)
{
    if (name == NULL || FindInput(name) != NULL || value.type != type)
        return B_BAD_VALUE;
    OperatorInput* input = new(std::nothrow) OperatorInput(name, type, value, selectorNames, count);
    if (input == NULL || !fInputs.AddItem(input)) { delete input; return B_NO_MEMORY; }
    return B_OK;
}
status_t Operator::AddOutput(const char* name, OperatorValue::Type type,
    OperatorOutputFunction function)
{
    if (name == NULL || FindOutput(name) != NULL || function == NULL)
        return B_BAD_VALUE;
    OperatorOutput* output = new(std::nothrow) OperatorOutput(this, name, type, function);
    if (output == NULL || !fOutputs.AddItem(output)) { delete output; return B_NO_MEMORY; }
    return B_OK;
}
status_t Operator::SetInputDefault(const char* name, const OperatorValue& value)
{
    OperatorInput* input = FindInput(name);
    if (input == NULL || input->type != value.type || value.type == OperatorValue::EVENT_VALUE)
        return B_BAD_VALUE;
    input->defaultValue = value;
    return B_OK;
}

status_t Operator::Archive(BMessage* into, bool deep) const
{
    status_t status = BArchivable::Archive(into, deep);
    if (status == B_OK) status = into->AddString("operator:type", ClassName());
    if (status == B_OK) status = into->AddInt32("operator:id", fID);
    if (status == B_OK) status = into->AddPoint("operator:position", fPosition);
    for (int32 i = 0; status == B_OK && i < InputCount(); i++) {
        const OperatorInput* input = fInputs.ItemAt(i);
        if (input->type == OperatorValue::EVENT_VALUE) continue;
        BMessage value;
        status = value.AddString("name", input->name);
        if (status == B_OK) status = value.AddInt32("type", input->type);
        if (status == B_OK) {
            switch (input->type) {
                case OperatorValue::FLOAT_VALUE: status = value.AddFloat("value", input->defaultValue.data.floatValue); break;
                case OperatorValue::BOOL_VALUE: status = value.AddBool("value", input->defaultValue.data.boolValue); break;
                case OperatorValue::SELECTOR_VALUE: status = value.AddInt32("value", input->defaultValue.data.selectorValue); break;
                case OperatorValue::TIME_VALUE: status = value.AddUInt64("value_usec", input->defaultValue.data.timeValue); break;
                case OperatorValue::EVENT_VALUE: break;
            }
        }
        if (status == B_OK) status = into->AddMessage("operator:input", &value);
    }
    return status;
}

status_t Operator::Unarchive(const BMessage* archive)
{
    archive->FindPoint("operator:position", &fPosition);
    archive->FindInt32("operator:id", &fID);
    BMessage message;
    for (int32 i = 0; archive->FindMessage("operator:input", i, &message) == B_OK; i++) {
        const char* name;
        int32 type;
        if (message.FindString("name", &name) != B_OK || message.FindInt32("type", &type) != B_OK)
            continue;
        OperatorInput* input = FindInput(name);
        if (input == NULL || input->type != type || input->type == OperatorValue::EVENT_VALUE)
            continue;
        OperatorValue value = input->defaultValue;
        status_t status = B_BAD_DATA;
        switch (input->type) {
            case OperatorValue::FLOAT_VALUE: status = message.FindFloat("value", &value.data.floatValue); break;
            case OperatorValue::BOOL_VALUE: status = message.FindBool("value", &value.data.boolValue); break;
            case OperatorValue::SELECTOR_VALUE: status = message.FindInt32("value", &value.data.selectorValue); break;
            case OperatorValue::TIME_VALUE: status = message.FindUInt64("value_usec", &value.data.timeValue); break;
            case OperatorValue::EVENT_VALUE: break;
        }
        if (status == B_OK) input->defaultValue = value;
    }
    return B_OK;
}

void Operator::ReceiveEvent(OperatorInput*, OperatorVoiceNote&, OperatorState&, uint64) { }
BArchivable* Operator::Instantiate(BMessage* archive)
{
    if (archive == NULL) return NULL;
    const char* type;
    if (archive->FindString("operator:type", &type) != B_OK) return NULL;
    return InstantiateOperator(type, archive);
}

NoteOperator::NoteOperator() : Operator()
{
    AddOutput("Frequency", OperatorValue::FLOAT_VALUE, NoteFrequency);
    AddOutput("Velocity", OperatorValue::FLOAT_VALUE, NoteVelocity);
    AddOutput("NoteOff", OperatorValue::EVENT_VALUE, NoteOffValue);
}
NoteOperator::NoteOperator(BMessage* archive) : Operator(archive)
    { AddOutput("Frequency", OperatorValue::FLOAT_VALUE, NoteFrequency); AddOutput("Velocity", OperatorValue::FLOAT_VALUE, NoteVelocity); AddOutput("NoteOff", OperatorValue::EVENT_VALUE, NoteOffValue); Unarchive(archive); }
const char* NoteOperator::ClassName() const { return "NoteOperator"; }
OperatorState* NoteOperator::CreateState() const { return new(std::nothrow) NoteState; }

SineOperator::SineOperator() : Operator()
{
    AddInput("Frequency", OperatorValue::FLOAT_VALUE, OperatorValue::Float(440.0f));
    AddInput("Amplitude", OperatorValue::FLOAT_VALUE, OperatorValue::Float(1.0f));
    AddOutput("Sample", OperatorValue::FLOAT_VALUE, SineSample);
}
SineOperator::SineOperator(BMessage* archive) : Operator(archive)
{
    AddInput("Frequency", OperatorValue::FLOAT_VALUE, OperatorValue::Float(440.0f));
    AddInput("Amplitude", OperatorValue::FLOAT_VALUE, OperatorValue::Float(1.0f));
    AddOutput("Sample", OperatorValue::FLOAT_VALUE, SineSample);
    Unarchive(archive);
}
const char* SineOperator::ClassName() const { return "SineOperator"; }
OperatorState* SineOperator::CreateState() const { return new(std::nothrow) SineState; }

EnvelopeOperator::EnvelopeOperator() : Operator()
{
    AddInput("Attack", OperatorValue::TIME_VALUE, OperatorValue::Time(0));
    AddInput("Decay", OperatorValue::TIME_VALUE, OperatorValue::Time(500000));
    AddInput("Sustain", OperatorValue::FLOAT_VALUE, OperatorValue::Float(1.0f));
    AddInput("Release", OperatorValue::TIME_VALUE, OperatorValue::Time(100000));
    AddInput("NoteOff", OperatorValue::EVENT_VALUE, OperatorValue::Event());
    AddOutput("Amplitude", OperatorValue::FLOAT_VALUE, EnvelopeAmplitude);
}
EnvelopeOperator::EnvelopeOperator(BMessage* archive) : Operator(archive)
{
    AddInput("Attack", OperatorValue::TIME_VALUE, OperatorValue::Time(0));
    AddInput("Decay", OperatorValue::TIME_VALUE, OperatorValue::Time(500000));
    AddInput("Sustain", OperatorValue::FLOAT_VALUE, OperatorValue::Float(1.0f));
    AddInput("Release", OperatorValue::TIME_VALUE, OperatorValue::Time(100000));
    AddInput("NoteOff", OperatorValue::EVENT_VALUE, OperatorValue::Event());
    AddOutput("Amplitude", OperatorValue::FLOAT_VALUE, EnvelopeAmplitude);
    Unarchive(archive);
}
const char* EnvelopeOperator::ClassName() const { return "EnvelopeOperator"; }
OperatorState* EnvelopeOperator::CreateState() const { return new(std::nothrow) EnvelopeState; }
bool EnvelopeOperator::IsFinished(const OperatorState& state) const
    { return static_cast<const EnvelopeState&>(state).finished; }
void EnvelopeOperator::ReceiveEvent(OperatorInput* input, OperatorVoiceNote& note,
    OperatorState& base, uint64 frame)
{
    if (input == NULL || input->name != "NoteOff") return;
    EnvelopeState& state = static_cast<EnvelopeState&>(base);
    if (!state.released) {
        state.releaseAmplitude = note.Pull(FindOutput("Amplitude")).data.floatValue;
        state.releaseFrame = frame;
        state.released = true;
    }
}

NoiseOperator::NoiseOperator() : Operator()
    { AddOutput("Sample", OperatorValue::FLOAT_VALUE, NoiseSample); }
NoiseOperator::NoiseOperator(BMessage* archive) : Operator(archive)
    { AddOutput("Sample", OperatorValue::FLOAT_VALUE, NoiseSample); Unarchive(archive); }
const char* NoiseOperator::ClassName() const { return "NoiseOperator"; }
OperatorState* NoiseOperator::CreateState() const { return new(std::nothrow) NoiseState; }

OperatorVoice::OperatorVoice(float sampleRate)
    : Voice(sampleRate), fOperators(), fNextID(1), fNoteOperatorID(-1),
      fAudioOutput(NULL), fAudioDefault(OperatorValue::Float(0.0f))
    { fNoteOperatorID = AddOperator(new(std::nothrow) NoteOperator); }
OperatorVoice::OperatorVoice(BMessage* archive)
    : Voice(archive), fOperators(), fNextID(1), fNoteOperatorID(-1),
      fAudioOutput(NULL), fAudioDefault(OperatorValue::Float(0.0f))
    { _Unarchive(archive); }
OperatorVoice::~OperatorVoice() { }
const char* OperatorVoice::Name() { return "Operator Voice"; }
BView* OperatorVoice::CreateParamView() { return Voice::CreateParamView(); }

int32 OperatorVoice::AddOperator(Operator* op)
{
    if (op == NULL || fVoiceNotes.CountItems() != 0) { delete op; return -1; }
    if (dynamic_cast<NoteOperator*>(op) != NULL && fNoteOperatorID >= 0) { delete op; return -1; }
    op->fID = fNextID++;
    if (!fOperators.AddItem(op)) { delete op; return -1; }
    if (dynamic_cast<NoteOperator*>(op) != NULL) fNoteOperatorID = op->fID;
    _RefreshGraphIndices();
    return op->fID;
}
Operator* OperatorVoice::OperatorAt(int32 index) const { return fOperators.ItemAt(index); }
int32 OperatorVoice::CountOperators() const { return fOperators.CountItems(); }
Operator* OperatorVoice::FindOperator(int32 id) const
{
    for (int32 i = 0; i < CountOperators(); i++)
        if (fOperators.ItemAt(i)->ID() == id) return fOperators.ItemAt(i);
    return NULL;
}
void OperatorVoice::_RefreshGraphIndices()
{
    int32 cacheIndex = 0;
    for (int32 i = 0; i < CountOperators(); i++) {
        Operator* op = OperatorAt(i);
        op->fGraphIndex = i;
        for (int32 j = 0; j < op->OutputCount(); j++)
            op->OutputAt(j)->cacheIndex = cacheIndex++;
    }
}

status_t OperatorVoice::Connect(int32 sourceID, const char* outputName,
    int32 targetID, const char* inputName)
{
    Operator* source = FindOperator(sourceID);
    Operator* target = FindOperator(targetID);
    if (source == NULL || target == NULL) return B_BAD_VALUE;
    OperatorOutput* output = source->FindOutput(outputName);
    OperatorInput* input = target->FindInput(inputName);
    if (output == NULL || input == NULL || output->type != input->type) return B_BAD_VALUE;
    Disconnect(targetID, inputName);
    input->source = output;
    if (input->type == OperatorValue::EVENT_VALUE)
        output->eventTargets.AddItem(input);
    return B_OK;
}
status_t OperatorVoice::Connect(int32 sourceID, int32 sourceOutput,
    int32 targetID, int32 targetInput)
{
    Operator* source = FindOperator(sourceID); Operator* target = FindOperator(targetID);
    if (source == NULL || target == NULL || sourceOutput < 0 || sourceOutput >= source->OutputCount()
        || targetInput < 0 || targetInput >= target->InputCount()) return B_BAD_VALUE;
    return Connect(sourceID, source->OutputAt(sourceOutput)->name,
        targetID, target->InputAt(targetInput)->name);
}
void OperatorVoice::Disconnect(int32 targetID, const char* inputName)
{
    Operator* target = FindOperator(targetID);
    if (target == NULL) return;
    OperatorInput* input = target->FindInput(inputName);
    if (input == NULL || input->source == NULL) return;
    if (input->type == OperatorValue::EVENT_VALUE)
        input->source->eventTargets.RemoveItem(input);
    input->source = NULL;
}
int32 OperatorVoice::NoteOperatorID() const { return fNoteOperatorID; }
status_t OperatorVoice::SetAudioOutput(int32 sourceID, const char* outputName)
{
    Operator* source = FindOperator(sourceID);
    OperatorOutput* output = source != NULL ? source->FindOutput(outputName) : NULL;
    if (output == NULL || output->type != OperatorValue::FLOAT_VALUE) return B_BAD_VALUE;
    fAudioOutput = output;
    return B_OK;
}
status_t OperatorVoice::SetAudioOutput(int32 sourceID, int32 outputIndex)
{
    Operator* source = FindOperator(sourceID);
    if (source == NULL || outputIndex < 0 || outputIndex >= source->OutputCount()) return B_BAD_VALUE;
    return SetAudioOutput(sourceID, source->OutputAt(outputIndex)->name);
}
void OperatorVoice::ClearAudioOutput() { fAudioOutput = NULL; }
bool OperatorVoice::HasAudioOutput() const { return fAudioOutput != NULL; }
bool OperatorVoice::HasEnvelopes() const
{
    for (int32 i = 0; i < CountOperators(); i++)
        if (dynamic_cast<EnvelopeOperator*>(OperatorAt(i)) != NULL) return true;
    return false;
}
VoiceNote* OperatorVoice::createNote(uint8 note, float frequency, uint8 velocity, bigtime_t time)
    { return new(std::nothrow) OperatorVoiceNote(this, note, frequency, velocity, time); }

status_t OperatorVoice::Archive(BMessage* into, bool deep) const
{
    status_t status = Voice::Archive(into, deep);
    if (status == B_OK) status = into->AddInt32("operatorVoice:next_id", fNextID);
    if (status == B_OK) status = into->AddInt32("operatorVoice:note_id", fNoteOperatorID);
    if (status == B_OK) status = into->AddFloat("operatorVoice:audio_default", fAudioDefault.data.floatValue);
    if (status == B_OK && fAudioOutput != NULL) {
        status = into->AddInt32("operatorVoice:audio_id", fAudioOutput->owner->ID());
        if (status == B_OK) status = into->AddString("operatorVoice:audio_name", fAudioOutput->name);
    }
    for (int32 i = 0; status == B_OK && i < CountOperators(); i++) {
        Operator* op = OperatorAt(i);
        BMessage opArchive;
        status = op->Archive(&opArchive, deep);
        if (status == B_OK) status = into->AddMessage("operatorVoice:operator", &opArchive);
        for (int32 j = 0; status == B_OK && j < op->InputCount(); j++) {
            OperatorInput* input = op->InputAt(j);
            if (input->source == NULL) continue;
            BMessage link;
            status = link.AddInt32("source_id", input->source->owner->ID());
            if (status == B_OK) status = link.AddString("source_output", input->source->name);
            if (status == B_OK) status = link.AddInt32("target_id", op->ID());
            if (status == B_OK) status = link.AddString("target_input", input->name);
            if (status == B_OK) status = into->AddMessage("operatorVoice:connection", &link);
        }
    }
    return status;
}

status_t OperatorVoice::_Unarchive(const BMessage* archive)
{
    archive->FindInt32("operatorVoice:next_id", &fNextID);
    archive->FindInt32("operatorVoice:note_id", &fNoteOperatorID);
    archive->FindFloat("operatorVoice:audio_default", &fAudioDefault.data.floatValue);
    BMessage opArchive;
    for (int32 i = 0; archive->FindMessage("operatorVoice:operator", i, &opArchive) == B_OK; i++) {
        const char* type;
        if (opArchive.FindString("operator:type", &type) != B_OK) continue;
        Operator* op = InstantiateOperator(type, &opArchive);
        if (op == NULL) continue;
        op->Unarchive(&opArchive);
        int32 id = op->ID();
        if (id < 0 || FindOperator(id) != NULL || !fOperators.AddItem(op)) { delete op; continue; }
        if (id >= fNextID) fNextID = id + 1;
    }
    _RefreshGraphIndices();
    BMessage link;
    for (int32 i = 0; archive->FindMessage("operatorVoice:connection", i, &link) == B_OK; i++) {
        int32 sourceID, targetID; const char *outputName, *inputName;
        if (link.FindInt32("source_id", &sourceID) == B_OK
            && link.FindString("source_output", &outputName) == B_OK
            && link.FindInt32("target_id", &targetID) == B_OK
            && link.FindString("target_input", &inputName) == B_OK)
            Connect(sourceID, outputName, targetID, inputName);
    }
    int32 audioID; const char* audioName;
    if (archive->FindInt32("operatorVoice:audio_id", &audioID) == B_OK
        && archive->FindString("operatorVoice:audio_name", &audioName) == B_OK)
        SetAudioOutput(audioID, audioName);
    if (FindOperator(fNoteOperatorID) == NULL) {
        fNoteOperatorID = -1;
        for (int32 i = 0; i < CountOperators(); i++)
            if (dynamic_cast<NoteOperator*>(OperatorAt(i)) != NULL) fNoteOperatorID = OperatorAt(i)->ID();
        if (fNoteOperatorID < 0) fNoteOperatorID = AddOperator(new(std::nothrow) NoteOperator);
    }
    return B_OK;
}
BArchivable* OperatorVoice::Instantiate(BMessage* archive)
{
    if (archive == NULL || !validate_instantiation(archive, "OperatorVoice")) return NULL;
    return new(std::nothrow) OperatorVoice(archive);
}
void OperatorVoice::_SendNoteOff(OperatorVoiceNote& note, uint64 frame)
{
    Operator* noteOperator = FindOperator(fNoteOperatorID);
    OperatorOutput* event = noteOperator != NULL ? noteOperator->FindOutput("NoteOff") : NULL;
    if (event == NULL) return;
    for (int32 i = 0; i < event->eventTargets.CountItems(); i++) {
        OperatorInput* input = event->eventTargets.ItemAt(i);
        Operator* target = FindOperator(input->source != NULL ? input->source->owner->ID() : -1);
        if (target == NULL) continue;
        OperatorState* state = note.fStates.ItemAt(target->fGraphIndex);
        if (state != NULL) target->ReceiveEvent(input, note, *state, frame);
    }
}

OperatorVoiceNote::OperatorVoiceNote(OperatorVoice* voice, uint8 note, float frequency,
    uint8 velocity, bigtime_t time)
    : VoiceNote(voice, note, frequency, velocity, time), fGraph(voice), fFrame(0), fStates(), fCache()
{
    int32 outputCount = 0;
    for (int32 i = 0; i < voice->CountOperators(); i++) {
        Operator* op = voice->OperatorAt(i);
        OperatorState* state = op->CreateState();
        if (state == NULL || !fStates.AddItem(state)) { delete state; break; }
        outputCount += op->OutputCount();
    }
    for (int32 i = 0; i < outputCount; i++) {
        OperatorVoiceNote::OutputCache* cache = new(std::nothrow) OutputCache;
        if (cache == NULL || !fCache.AddItem(cache)) { delete cache; break; }
    }
}
uint64 OperatorVoiceNote::Frame() const { return fFrame; }
float OperatorVoiceNote::NoteFrequency() const { return fFrequency; }
float OperatorVoiceNote::NoteVelocity() const { return fVelocity; }
OperatorValue OperatorVoiceNote::InputValue(const Operator* op, const char* name)
{
    OperatorInput* input = op != NULL ? op->FindInput(name) : NULL;
    if (input == NULL) return OperatorValue::Float(0.0f);
    if (input->type == OperatorValue::EVENT_VALUE) return OperatorValue::Event();
    if (input->source != NULL) return Pull(input->source);
    OperatorValue value = input->defaultValue;
    if (value.type == OperatorValue::TIME_VALUE)
        value.data.timeValue = TimeToFrames(value.data.timeValue, fSampleRate);
    return value;
}
OperatorValue OperatorVoiceNote::Pull(OperatorOutput* output)
{
    if (output == NULL || output->cacheIndex < 0 || output->cacheIndex >= fCache.CountItems())
        return OperatorValue::Float(0.0f);
    OutputCache* cache = fCache.ItemAt(output->cacheIndex);
    if (cache->frame == fFrame && (cache->valid || cache->evaluating)) return cache->value;
    Operator* owner = output->owner;
    if (owner == NULL || owner->fGraphIndex < 0 || owner->fGraphIndex >= fStates.CountItems())
        return OperatorValue::Float(0.0f);
    cache->frame = fFrame; cache->evaluating = true; cache->valid = false;
    cache->value = output->function(owner, output->cacheIndex, *this, *fStates.ItemAt(owner->fGraphIndex));
    cache->evaluating = false; cache->valid = true;
    return cache->value;
}
void OperatorVoiceNote::NoteOff(bigtime_t eventTime)
{
    VoiceNote::NoteOff(eventTime);
    fGraph->_SendNoteOff(*this, fFrame);
    if (!fGraph->HasEnvelopes()) Recycle();
}
float OperatorVoiceNote::NextSample()
{
    if (fStates.CountItems() != fGraph->CountOperators()) { Recycle(); return 0.0f; }
    float sample = fGraph->fAudioDefault.data.floatValue;
    if (fGraph->fAudioOutput != NULL) sample = Pull(fGraph->fAudioOutput).data.floatValue;
    bool allFinished = true;
    if (fState == RELEASE && fGraph->HasEnvelopes()) {
        for (int32 i = 0; i < fGraph->CountOperators(); i++) {
            EnvelopeOperator* envelope = dynamic_cast<EnvelopeOperator*>(fGraph->OperatorAt(i));
            if (envelope != NULL) {
                Pull(envelope->FindOutput("Amplitude"));
                if (!envelope->IsFinished(*fStates.ItemAt(i))) allFinished = false;
            }
        }
        if (allFinished) { Recycle(); return 0.0f; }
    }
    fFrame++;
    return sample;
}
