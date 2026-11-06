/*
 * Copyright 2026, Fishpond
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "OperatorVoice.h"

#include <math.h>
#include <Message.h>
#include <String.h>

#include <new>

namespace {

const OperatorPort kNoInputs[] = { };
const OperatorPort kNoteOutputs[] = {
    { "Frequency", OperatorValue::FLOAT_VALUE },
    { "Velocity", OperatorValue::FLOAT_VALUE },
    { "NoteOff", OperatorValue::EVENT_VALUE }
};
const OperatorPort kSineInputs[] = {
    { "Frequency", OperatorValue::FLOAT_VALUE },
    { "Amplitude", OperatorValue::FLOAT_VALUE }
};
const OperatorPort kSineOutputs[] = {
    { "Sample", OperatorValue::FLOAT_VALUE }
};
const OperatorPort kEnvelopeInputs[] = {
    { "Attack", OperatorValue::TIME_VALUE },
    { "Decay", OperatorValue::TIME_VALUE },
    { "Sustain", OperatorValue::FLOAT_VALUE },
    { "Release", OperatorValue::TIME_VALUE },
    { "NoteOff", OperatorValue::EVENT_VALUE }
};
const OperatorPort kEnvelopeOutputs[] = {
    { "Amplitude", OperatorValue::FLOAT_VALUE }
};
const OperatorPort kNoiseOutputs[] = {
    { "Sample", OperatorValue::FLOAT_VALUE }
};

struct BasicState : public OperatorState {
    float phase;
    BasicState() : phase(0.0f) { }
};

struct EnvelopeState : public OperatorState {
    uint64 releaseFrame;
    float releaseAmplitude;
    bool released;
    bool finished;
    EnvelopeState()
        : releaseFrame(0), releaseAmplitude(0.0f), released(false), finished(false) { }
};

struct NoiseState : public OperatorState {
    uint32 random;
    NoiseState() : random(0x9e3779b9) { }
};

uint64
TimeToFrames(uint64 microseconds, float sampleRate)
{
    if (microseconds == 0 || sampleRate <= 0.0f)
        return 0;
    double frames = (double)microseconds * (double)sampleRate / 1000000.0;
    return frames >= (double)UINT64_MAX ? UINT64_MAX : (uint64)frames;
}

Operator* CreateOperator(const char* type, BMessage* archive)
{
    if (strcmp(type, "NoteOperator") == 0)
        return new(std::nothrow) NoteOperator(archive);
    if (strcmp(type, "SineOperator") == 0)
        return new(std::nothrow) SineOperator(archive);
    if (strcmp(type, "EnvelopeOperator") == 0)
        return new(std::nothrow) EnvelopeOperator(archive);
    if (strcmp(type, "NoiseOperator") == 0)
        return new(std::nothrow) NoiseOperator(archive);
    return NULL;
}

} // namespace

OperatorValue OperatorValue::Float(float value)
{
    OperatorValue result;
    result.type = FLOAT_VALUE;
    result.data.floatValue = value;
    return result;
}

OperatorValue OperatorValue::Event(uint64 frame)
{
    OperatorValue result;
    result.type = EVENT_VALUE;
    result.data.timeValue = frame;
    return result;
}

OperatorValue OperatorValue::Bool(bool value)
{
    OperatorValue result;
    result.type = BOOL_VALUE;
    result.data.boolValue = value;
    return result;
}

OperatorValue OperatorValue::Selector(int32 value)
{
    OperatorValue result;
    result.type = SELECTOR_VALUE;
    result.data.selectorValue = value;
    return result;
}

OperatorValue OperatorValue::Time(uint64 value)
{
    OperatorValue result;
    result.type = TIME_VALUE;
    result.data.timeValue = value;
    return result;
}

Operator::Operator(const OperatorPort* inputs, int32 inputCount,
    const OperatorPort* outputs, int32 outputCount)
    :
    BArchivable(),
    fPosition(0.0f, 0.0f)
{
    for (int32 i = 0; i < inputCount; i++) {
        fInputs.push_back(inputs[i]);
        OperatorValue value;
        value.type = inputs[i].type;
        switch (value.type) {
            case OperatorValue::FLOAT_VALUE: value.data.floatValue = 0.0f; break;
            case OperatorValue::EVENT_VALUE: value.data.timeValue = 0; break;
            case OperatorValue::BOOL_VALUE: value.data.boolValue = false; break;
            case OperatorValue::SELECTOR_VALUE: value.data.selectorValue = 0; break;
            case OperatorValue::TIME_VALUE: value.data.timeValue = 0; break;
        }
        fDefaults.push_back(value);
    }
    for (int32 i = 0; i < outputCount; i++)
        fOutputs.push_back(outputs[i]);
}

Operator::Operator(BMessage* archive)
    :
    BArchivable(archive),
    fPosition(0.0f, 0.0f)
{
}

Operator::~Operator()
{
}

void Operator::ReceiveEvent(int32, OperatorVoiceNote&, OperatorState&, uint64)
{
}

int32 Operator::InputCount() const { return (int32)fInputs.size(); }
int32 Operator::OutputCount() const { return (int32)fOutputs.size(); }
const OperatorPort& Operator::Input(int32 index) const { return fInputs[index]; }
const OperatorPort& Operator::Output(int32 index) const { return fOutputs[index]; }
const OperatorValue& Operator::InputDefault(int32 index) const { return fDefaults[index]; }
BPoint Operator::Position() const { return fPosition; }
void Operator::SetPosition(BPoint position) { fPosition = position; }

status_t
Operator::SetInputDefault(int32 index, const OperatorValue& value)
{
    if (index < 0 || index >= InputCount() || fInputs[index].type != value.type
        || value.type == OperatorValue::EVENT_VALUE)
        return B_BAD_VALUE;
    fDefaults[index] = value;
    return B_OK;
}

status_t
Operator::Archive(BMessage* into, bool deep) const
{
    status_t status = BArchivable::Archive(into, deep);
    if (status != B_OK)
        return status;
    status = into->AddString("operator:type", ClassName());
    if (status == B_OK)
        status = into->AddPoint("operator:position", fPosition);
    for (int32 i = 0; status == B_OK && i < InputCount(); i++) {
        if (fDefaults[i].type == OperatorValue::EVENT_VALUE)
            continue;
        BMessage value;
        status = value.AddInt32("index", i);
        if (status == B_OK)
            status = value.AddInt32("type", fDefaults[i].type);
        if (status != B_OK)
            break;
        switch (fDefaults[i].type) {
            case OperatorValue::FLOAT_VALUE:
                status = value.AddFloat("value", fDefaults[i].data.floatValue); break;
            case OperatorValue::BOOL_VALUE:
                status = value.AddBool("value", fDefaults[i].data.boolValue); break;
            case OperatorValue::SELECTOR_VALUE:
                status = value.AddInt32("value", fDefaults[i].data.selectorValue); break;
            case OperatorValue::TIME_VALUE:
                // Time values are persisted in microseconds, not frame counts.
                status = value.AddUInt64("value", fDefaults[i].data.timeValue); break;
            case OperatorValue::EVENT_VALUE:
                break;
        }
        if (status == B_OK)
            status = into->AddMessage("operator:input", &value);
    }
    return status;
}

status_t
Operator::Unarchive(const BMessage* archive)
{
    archive->FindPoint("operator:position", &fPosition);
    BMessage value;
    for (int32 i = 0; archive->FindMessage("operator:input", i, &value) == B_OK; i++) {
        int32 index;
        int32 type;
        if (value.FindInt32("index", &index) != B_OK
            || value.FindInt32("type", &type) != B_OK
            || index < 0 || index >= InputCount() || fInputs[index].type != type)
            continue;
        OperatorValue restored;
        restored.type = fInputs[index].type;
        status_t status = B_BAD_DATA;
        switch (restored.type) {
            case OperatorValue::FLOAT_VALUE:
                status = value.FindFloat("value", &restored.data.floatValue); break;
            case OperatorValue::BOOL_VALUE:
                status = value.FindBool("value", &restored.data.boolValue); break;
            case OperatorValue::SELECTOR_VALUE:
                status = value.FindInt32("value", &restored.data.selectorValue); break;
            case OperatorValue::TIME_VALUE:
                status = value.FindUInt64("value", &restored.data.timeValue); break;
            case OperatorValue::EVENT_VALUE:
                break;
        }
        if (status == B_OK)
            fDefaults[index] = restored;
    }
    return B_OK;
}

NoteOperator::NoteOperator()
    : Operator(kNoInputs, 0, kNoteOutputs, 3) { }
NoteOperator::NoteOperator(BMessage* archive)
    : Operator(archive) { }
const char* NoteOperator::ClassName() const { return "NoteOperator"; }
std::unique_ptr<OperatorState> NoteOperator::CreateState() const
    { return std::unique_ptr<OperatorState>(new OperatorState); }
OperatorValue NoteOperator::EvaluateOutput(int32 output, OperatorVoiceNote& note,
    OperatorState&) const
{
    if (output == 0) return OperatorValue::Float(note.NoteFrequency());
    if (output == 1) return OperatorValue::Float(note.NoteVelocity());
    if (output == 2) return OperatorValue::Event(note.Frame());
    return OperatorValue::Float(0.0f);
}

SineOperator::SineOperator()
    : Operator(kSineInputs, 2, kSineOutputs, 1)
{
    SetInputDefault(0, OperatorValue::Float(440.0f));
    SetInputDefault(1, OperatorValue::Float(1.0f));
}
SineOperator::SineOperator(BMessage* archive) : Operator(archive) { }
const char* SineOperator::ClassName() const { return "SineOperator"; }
std::unique_ptr<OperatorState> SineOperator::CreateState() const
    { return std::unique_ptr<OperatorState>(new BasicState); }
OperatorValue SineOperator::EvaluateOutput(int32 output, OperatorVoiceNote& note,
    OperatorState& baseState) const
{
    if (output != 0) return OperatorValue::Float(0.0f);
    BasicState& state = static_cast<BasicState&>(baseState);
    OperatorValue frequency = note.InputValue(this, 0);
    OperatorValue amplitude = note.InputValue(this, 1);
    state.phase += (float)(2.0 * M_PI) * frequency.data.floatValue / note.SampleRate();
    if (state.phase >= (float)(2.0 * M_PI))
        state.phase = fmodf(state.phase, (float)(2.0 * M_PI));
    return OperatorValue::Float(sinf(state.phase) * amplitude.data.floatValue);
}

EnvelopeOperator::EnvelopeOperator()
    : Operator(kEnvelopeInputs, 5, kEnvelopeOutputs, 1)
{
    SetInputDefault(0, OperatorValue::Time(0));
    SetInputDefault(1, OperatorValue::Time(500000));
    SetInputDefault(2, OperatorValue::Float(1.0f));
    SetInputDefault(3, OperatorValue::Time(100000));
}
EnvelopeOperator::EnvelopeOperator(BMessage* archive) : Operator(archive) { }
const char* EnvelopeOperator::ClassName() const { return "EnvelopeOperator"; }
std::unique_ptr<OperatorState> EnvelopeOperator::CreateState() const
    { return std::unique_ptr<OperatorState>(new EnvelopeState); }
void EnvelopeOperator::ReceiveEvent(int32 input, OperatorVoiceNote& note,
    OperatorState& baseState, uint64 frame)
{
    if (input != 4)
        return;
    EnvelopeState& state = static_cast<EnvelopeState&>(baseState);
    if (!state.released) {
        state.releaseAmplitude = EvaluateOutput(0, note, baseState).data.floatValue;
        state.releaseFrame = frame;
        state.released = true;
    }
}
OperatorValue EnvelopeOperator::EvaluateOutput(int32 output, OperatorVoiceNote& note,
    OperatorState& baseState) const
{
    if (output != 0) return OperatorValue::Float(0.0f);
    EnvelopeState& state = static_cast<EnvelopeState&>(baseState);
    if (state.finished) return OperatorValue::Float(0.0f);

    uint64 frame = note.Frame();
    uint64 attack = TimeToFrames(note.InputValue(this, 0).data.timeValue, note.SampleRate());
    uint64 decay = TimeToFrames(note.InputValue(this, 1).data.timeValue, note.SampleRate());
    float sustain = note.InputValue(this, 2).data.floatValue;
    uint64 release = TimeToFrames(note.InputValue(this, 3).data.timeValue, note.SampleRate());

    float amplitude;
    if (state.released) {
        uint64 elapsed = frame >= state.releaseFrame ? frame - state.releaseFrame : 0;
        if (release == 0 || elapsed >= release) {
            state.finished = true;
            return OperatorValue::Float(0.0f);
        }
        amplitude = state.releaseAmplitude * (1.0f - (float)elapsed / (float)release);
    } else if (attack > 0 && frame < attack) {
        amplitude = (float)frame / (float)attack;
    } else if (decay > 0 && frame < attack + decay) {
        float progress = (float)(frame - attack) / (float)decay;
        amplitude = 1.0f + (sustain - 1.0f) * progress;
    } else {
        amplitude = sustain;
    }
    return OperatorValue::Float(amplitude);
}
bool EnvelopeOperator::IsFinished(const OperatorState& state) const
    { return static_cast<const EnvelopeState&>(state).finished; }

NoiseOperator::NoiseOperator()
    : Operator(kNoInputs, 0, kNoiseOutputs, 1) { }
NoiseOperator::NoiseOperator(BMessage* archive) : Operator(archive) { }
const char* NoiseOperator::ClassName() const { return "NoiseOperator"; }
std::unique_ptr<OperatorState> NoiseOperator::CreateState() const
    { return std::unique_ptr<OperatorState>(new NoiseState); }
OperatorValue NoiseOperator::EvaluateOutput(int32 output, OperatorVoiceNote&,
    OperatorState& baseState) const
{
    if (output != 0) return OperatorValue::Float(0.0f);
    NoiseState& state = static_cast<NoiseState&>(baseState);
    state.random ^= state.random << 13;
    state.random ^= state.random >> 17;
    state.random ^= state.random << 5;
    return OperatorValue::Float((float)((int32)state.random) / 2147483648.0f);
}

OperatorVoice::OperatorVoice(float sampleRate)
    :
    Voice(sampleRate),
    fNextID(1),
    fNoteOperatorID(-1),
    fAudioSourceID(-1),
    fAudioSourceOutput(-1),
    fAudioDefault(OperatorValue::Float(0.0f))
{
    fNoteOperatorID = AddOperator(new(std::nothrow) NoteOperator);
}

OperatorVoice::OperatorVoice(BMessage* archive)
    :
    Voice(archive),
    fNextID(1),
    fNoteOperatorID(-1),
    fAudioSourceID(-1),
    fAudioSourceOutput(-1),
    fAudioDefault(OperatorValue::Float(0.0f))
{
    _Unarchive(archive);
}

OperatorVoice::~OperatorVoice() { }
const char* OperatorVoice::Name() { return "Operator Voice"; }

int32
OperatorVoice::AddOperator(Operator* op)
{
    if (op == NULL)
        return -1;
    if (dynamic_cast<NoteOperator*>(op) != NULL && fNoteOperatorID >= 0) {
        delete op;
        return -1;
    }
    int32 id = fNextID++;
    fOperators.push_back(Entry(id, op));
    if (dynamic_cast<NoteOperator*>(op) != NULL)
        fNoteOperatorID = id;
    return id;
}

Operator* OperatorVoice::OperatorAt(int32 index) const
    { return index < 0 || index >= CountOperators() ? NULL : fOperators[index].op.get(); }
int32 OperatorVoice::CountOperators() const { return (int32)fOperators.size(); }
int32 OperatorVoice::_IndexForID(int32 id) const
{
    for (int32 i = 0; i < CountOperators(); i++)
        if (fOperators[i].id == id) return i;
    return -1;
}
Operator* OperatorVoice::FindOperator(int32 id) const
    { int32 index = _IndexForID(id); return index < 0 ? NULL : fOperators[index].op.get(); }
int32 OperatorVoice::IDForOperator(const Operator* op) const
{
    for (int32 i = 0; i < CountOperators(); i++)
        if (fOperators[i].op.get() == op) return fOperators[i].id;
    return -1;
}

status_t
OperatorVoice::Connect(int32 sourceID, int32 sourceOutput, int32 targetID, int32 targetInput)
{
    Operator* source = FindOperator(sourceID);
    Operator* target = FindOperator(targetID);
    if (source == NULL || target == NULL || sourceOutput < 0
        || sourceOutput >= source->OutputCount() || targetInput < 0
        || targetInput >= target->InputCount()
        || source->Output(sourceOutput).type != target->Input(targetInput).type
        || source->Output(sourceOutput).type == OperatorValue::EVENT_VALUE)
        return B_BAD_VALUE;
    Disconnect(targetID, targetInput);
    fConnections.push_back({ sourceID, sourceOutput, targetID, targetInput });
    return B_OK;
}

void OperatorVoice::Disconnect(int32 targetID, int32 targetInput)
{
    for (std::vector<Connection>::iterator it = fConnections.begin(); it != fConnections.end();) {
        if (it->targetID == targetID && it->targetInput == targetInput)
            it = fConnections.erase(it);
        else
            ++it;
    }
}

int32 OperatorVoice::NoteOperatorID() const { return fNoteOperatorID; }
status_t OperatorVoice::SetAudioOutput(int32 sourceID, int32 sourceOutput)
{
    Operator* source = FindOperator(sourceID);
    if (source == NULL || sourceOutput < 0 || sourceOutput >= source->OutputCount()
        || source->Output(sourceOutput).type != OperatorValue::FLOAT_VALUE)
        return B_BAD_VALUE;
    fAudioSourceID = sourceID;
    fAudioSourceOutput = sourceOutput;
    return B_OK;
}
void OperatorVoice::ClearAudioOutput() { fAudioSourceID = fAudioSourceOutput = -1; }
bool OperatorVoice::HasAudioOutput() const { return FindOperator(fAudioSourceID) != NULL; }
bool OperatorVoice::HasEnvelopes() const
{
    for (int32 i = 0; i < CountOperators(); i++)
        if (dynamic_cast<EnvelopeOperator*>(fOperators[i].op.get()) != NULL) return true;
    return false;
}
const std::vector<OperatorVoice::Connection>& OperatorVoice::Connections() const
    { return fConnections; }

VoiceNote*
OperatorVoice::createNote(uint8 note, float frequency, uint8 velocity,
    bigtime_t performanceTime)
{
    return new(std::nothrow) OperatorVoiceNote(this, note, frequency, velocity, performanceTime);
}

status_t
OperatorVoice::Archive(BMessage* into, bool deep) const
{
    status_t status = Voice::Archive(into, deep);
    if (status != B_OK) return status;
    status = into->AddInt32("operatorVoice:next_id", fNextID);
    if (status == B_OK) status = into->AddInt32("operatorVoice:note_id", fNoteOperatorID);
    if (status == B_OK) status = into->AddInt32("operatorVoice:audio_source", fAudioSourceID);
    if (status == B_OK) status = into->AddInt32("operatorVoice:audio_output", fAudioSourceOutput);
    if (status == B_OK) status = into->AddFloat("operatorVoice:audio_default", fAudioDefault.data.floatValue);
    for (int32 i = 0; status == B_OK && i < CountOperators(); i++) {
        BMessage opArchive;
        status = fOperators[i].op->Archive(&opArchive, deep);
        if (status == B_OK) status = opArchive.AddInt32("operatorVoice:id", fOperators[i].id);
        if (status == B_OK) status = into->AddMessage("operatorVoice:operator", &opArchive);
    }
    for (size_t i = 0; status == B_OK && i < fConnections.size(); i++) {
        BMessage connection;
        status = connection.AddInt32("source", fConnections[i].sourceID);
        if (status == B_OK) status = connection.AddInt32("output", fConnections[i].sourceOutput);
        if (status == B_OK) status = connection.AddInt32("target", fConnections[i].targetID);
        if (status == B_OK) status = connection.AddInt32("input", fConnections[i].targetInput);
        if (status == B_OK) status = into->AddMessage("operatorVoice:connection", &connection);
    }
    return status;
}

status_t
OperatorVoice::_Unarchive(const BMessage* archive)
{
    archive->FindInt32("operatorVoice:next_id", &fNextID);
    archive->FindInt32("operatorVoice:note_id", &fNoteOperatorID);
    archive->FindInt32("operatorVoice:audio_source", &fAudioSourceID);
    archive->FindInt32("operatorVoice:audio_output", &fAudioSourceOutput);
    archive->FindFloat("operatorVoice:audio_default", &fAudioDefault.data.floatValue);
    BMessage opArchive;
    for (int32 i = 0; archive->FindMessage("operatorVoice:operator", i, &opArchive) == B_OK; i++) {
        const char* type;
        int32 id;
        if (opArchive.FindString("operator:type", &type) != B_OK
            || opArchive.FindInt32("operatorVoice:id", &id) != B_OK)
            continue;
        Operator* op = CreateOperator(type, &opArchive);
        if (op == NULL)
            continue;
        op->Unarchive(&opArchive);
        fOperators.push_back(Entry(id, op));
        if (id >= fNextID) fNextID = id + 1;
    }
    if (FindOperator(fNoteOperatorID) == NULL) {
        fNoteOperatorID = -1;
        for (int32 i = 0; i < CountOperators(); i++) {
            if (dynamic_cast<NoteOperator*>(fOperators[i].op.get()) != NULL) {
                fNoteOperatorID = fOperators[i].id;
                break;
            }
        }
        if (fNoteOperatorID < 0)
            fNoteOperatorID = AddOperator(new(std::nothrow) NoteOperator);
    }

    BMessage connection;
    for (int32 i = 0; archive->FindMessage("operatorVoice:connection", i, &connection) == B_OK; i++) {
        Connection item;
        if (connection.FindInt32("source", &item.sourceID) != B_OK
            || connection.FindInt32("output", &item.sourceOutput) != B_OK
            || connection.FindInt32("target", &item.targetID) != B_OK
            || connection.FindInt32("input", &item.targetInput) != B_OK)
            continue;
        Connect(item.sourceID, item.sourceOutput, item.targetID, item.targetInput);
    }
    return B_OK;
}

BArchivable*
OperatorVoice::Instantiate(BMessage* archive)
{
    if (!validate_instantiation(archive, "OperatorVoice"))
        return NULL;
    return new(std::nothrow) OperatorVoice(archive);
}

OperatorValue
OperatorVoice::_InputValue(OperatorVoiceNote& note, const Operator* op, int32 input)
{
    int32 targetID = IDForOperator(op);
    for (size_t i = 0; i < fConnections.size(); i++) {
        const Connection& connection = fConnections[i];
        if (connection.targetID == targetID && connection.targetInput == input) {
            Operator* source = FindOperator(connection.sourceID);
            if (source != NULL) return note.OutputValue(source, connection.sourceOutput);
        }
    }
    return op->InputDefault(input);
}

OperatorValue
OperatorVoice::_OutputValue(OperatorVoiceNote& note, const Operator* op, int32 output)
{
    return op->EvaluateOutput(output, note,
        *note.fStates[_IndexForID(IDForOperator(op))]);
}

void
OperatorVoice::_DispatchNoteOff(OperatorVoiceNote& note, uint64 frame)
{
    for (size_t i = 0; i < fConnections.size(); i++) {
        const Connection& connection = fConnections[i];
        if (connection.sourceID != fNoteOperatorID || connection.sourceOutput != 2)
            continue;
        Operator* target = FindOperator(connection.targetID);
        int32 index = _IndexForID(connection.targetID);
        if (target != NULL && index >= 0)
            target->ReceiveEvent(connection.targetInput, note, *note.fStates[index], frame);
    }
}

OperatorVoiceNote::OperatorVoiceNote(OperatorVoice* voice, uint8 note, float frequency,
    uint8 velocity, bigtime_t performanceTime)
    :
    VoiceNote(voice, note, frequency, velocity, performanceTime),
    fGraph(voice),
    fFrame(0)
{
    for (int32 i = 0; i < voice->CountOperators(); i++) {
        Operator* op = voice->OperatorAt(i);
        fStates.push_back(op->CreateState());
        fCache.push_back(std::vector<CachedOutput>(op->OutputCount()));
    }
}

uint64 OperatorVoiceNote::Frame() const { return fFrame; }
float OperatorVoiceNote::SampleRate() const { return fSampleRate; }
float OperatorVoiceNote::NoteFrequency() const { return fFrequency; }
float OperatorVoiceNote::NoteVelocity() const { return fVelocity; }
OperatorVoice* OperatorVoiceNote::Graph() const { return fGraph; }

OperatorValue OperatorVoiceNote::InputValue(const Operator* op, int32 input)
    { return fGraph->_InputValue(*this, op, input); }

OperatorValue
OperatorVoiceNote::OutputValue(const Operator* op, int32 output)
{
    int32 index = fGraph->_IndexForID(fGraph->IDForOperator(op));
    if (index < 0 || output < 0 || output >= op->OutputCount())
        return OperatorValue::Float(0.0f);
    CachedOutput& cache = fCache[index][output];
    if (cache.frame == fFrame && (cache.valid || cache.evaluating))
        return cache.value;
    cache.frame = fFrame;
    cache.evaluating = true;
    cache.valid = false;
    cache.value = op->EvaluateOutput(output, *this, *fStates[index]);
    cache.evaluating = false;
    cache.valid = true;
    return cache.value;
}

void
OperatorVoiceNote::NoteOff(bigtime_t eventTime)
{
    VoiceNote::NoteOff(eventTime);
    fGraph->_DispatchNoteOff(*this, fFrame);
    if (!fGraph->HasEnvelopes())
        Recycle();
}

float
OperatorVoiceNote::NextSample()
{
    if (fState == RELEASE && !fGraph->HasEnvelopes()) {
        Recycle();
        return 0.0f;
    }
    float sample = fGraph->fAudioDefault.data.floatValue;
    if (fGraph->HasAudioOutput()) {
        Operator* source = fGraph->FindOperator(fGraph->fAudioSourceID);
        if (source != NULL)
            sample = OutputValue(source, fGraph->fAudioSourceOutput).data.floatValue;
    }
    if (fState == RELEASE && fGraph->HasEnvelopes()) {
        bool allFinished = true;
        for (int32 i = 0; i < fGraph->CountOperators(); i++) {
            EnvelopeOperator* envelope = dynamic_cast<EnvelopeOperator*>(fGraph->OperatorAt(i));
            if (envelope != NULL && !envelope->IsFinished(*fStates[i])) {
                // Evaluate even if the envelope is not connected to the audio output.
                OutputValue(envelope, 0);
                if (!envelope->IsFinished(*fStates[i])) allFinished = false;
            }
        }
        if (allFinished) {
            Recycle();
            return 0.0f;
        }
    }
    fFrame++;
    return sample;
}
