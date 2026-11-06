/*
 * Copyright 2026, Fishpond
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef OPERATORVOICE_H
#define OPERATORVOICE_H

#include <Archivable.h>
#include <Message.h>
#include <ObjectList.h>
#include <Point.h>
#include <SupportDefs.h>
#include <String.h>

#include "Voice.h"

class Operator;
class OperatorInput;
class OperatorOutput;
class OperatorVoice;
class OperatorVoiceNote;

struct OperatorValue {
    enum Type { FLOAT_VALUE, EVENT_VALUE, BOOL_VALUE, SELECTOR_VALUE, TIME_VALUE } type;
    union {
        float floatValue;
        bool boolValue;
        int32 selectorValue;
        uint64 timeValue;
    } data;

    OperatorValue() : type(FLOAT_VALUE) { data.floatValue = 0.0f; }
    static OperatorValue Float(float value);
    static OperatorValue Event(uint64 frame = 0);
    static OperatorValue Bool(bool value);
    static OperatorValue Selector(int32 value);
    static OperatorValue Time(uint64 value);
};

class OperatorState {
public:
    virtual ~OperatorState() { }
};

typedef OperatorValue (*OperatorOutputFunction)(const Operator*, int32,
    OperatorVoiceNote&, OperatorState&);

class OperatorInput {
public:
    OperatorInput(const char* name, OperatorValue::Type type,
        const OperatorValue& defaultValue, const char* const* selectorNames,
        int32 selectorNameCount);

    BString name;
    OperatorValue::Type type;
    OperatorValue defaultValue;
    OperatorOutput* source;
    const char* const* selectorNames;
    int32 selectorNameCount;
};

class OperatorOutput {
public:
    OperatorOutput(Operator* owner, const char* name, OperatorValue::Type type,
        OperatorOutputFunction function);

    Operator* owner;
    BString name;
    OperatorValue::Type type;
    OperatorOutputFunction function;
    int32 cacheIndex;
    BObjectList<OperatorInput, false> eventTargets;
};

class Operator : public BArchivable {
public:
    virtual ~Operator();
    virtual const char* ClassName() const = 0;
    virtual OperatorState* CreateState() const = 0;
    virtual status_t Archive(BMessage* into, bool deep = true) const override;
    static BArchivable* Instantiate(BMessage* archive);

    int32 ID() const;
    BPoint Position() const;
    void SetPosition(BPoint position);
    int32 InputCount() const;
    int32 OutputCount() const;
    OperatorInput* InputAt(int32 index) const;
    OperatorOutput* OutputAt(int32 index) const;
    OperatorInput* FindInput(const char* name) const;
    OperatorOutput* FindOutput(const char* name) const;
    const char* const* SelectorNames(const char* inputName, int32* count) const;
    status_t SetInputDefault(const char* name, const OperatorValue& value);

protected:
    Operator();
    explicit Operator(BMessage* archive);
    status_t AddInput(const char* name, OperatorValue::Type type,
        const OperatorValue& defaultValue, const char* const* selectorNames = NULL,
        int32 selectorNameCount = 0);
    status_t AddOutput(const char* name, OperatorValue::Type type,
        OperatorOutputFunction function);
    status_t Unarchive(const BMessage* archive);
    virtual void ReceiveEvent(OperatorInput* input, OperatorVoiceNote& note,
        OperatorState& state, uint64 frame);

private:
    friend class OperatorVoice;
    friend class OperatorVoiceNote;
    int32 fID;
    int32 fGraphIndex;
    BPoint fPosition;
    BObjectList<OperatorInput, true> fInputs;
    BObjectList<OperatorOutput, true> fOutputs;
};

class NoteOperator : public Operator {
public:
    NoteOperator();
    explicit NoteOperator(BMessage* archive);
    virtual const char* ClassName() const override;
    virtual OperatorState* CreateState() const override;
};

class SineOperator : public Operator {
public:
    SineOperator();
    explicit SineOperator(BMessage* archive);
    virtual const char* ClassName() const override;
    virtual OperatorState* CreateState() const override;
};

class EnvelopeOperator : public Operator {
public:
    EnvelopeOperator();
    explicit EnvelopeOperator(BMessage* archive);
    virtual const char* ClassName() const override;
    virtual OperatorState* CreateState() const override;
    bool IsFinished(const OperatorState& state) const;
private:
    virtual void ReceiveEvent(OperatorInput* input, OperatorVoiceNote& note,
        OperatorState& state, uint64 frame) override;
};

class NoiseOperator : public Operator {
public:
    NoiseOperator();
    explicit NoiseOperator(BMessage* archive);
    virtual const char* ClassName() const override;
    virtual OperatorState* CreateState() const override;
};

// Keep the spelling used in the original feature request available as an alias.
typedef NoiseOperator NoiceOperator;

class OperatorVoiceNote : public VoiceNote {
friend class        OperatorVoice;
public:
    OperatorVoiceNote(OperatorVoice* voice, uint8 note, float frequency,
        uint8 velocity, bigtime_t performanceTime);
    virtual float NextSample() override;
    virtual void NoteOff(bigtime_t performanceTime) override;
    OperatorValue InputValue(const Operator* op, const char* inputName);
    OperatorValue Pull(OperatorOutput* output);
    uint64 Frame() const;
    float NoteFrequency() const;
    float NoteVelocity() const;

private:
    friend class Operator;
    struct OutputCache {
        uint64 frame;
        bool evaluating;
        bool valid;
        OperatorValue value;
        OutputCache() : frame(UINT64_MAX), evaluating(false), valid(false) { }
    };
    OperatorVoice* fGraph;
    uint64 fFrame;
    BObjectList<OperatorState, true> fStates;
    BObjectList<OutputCache, true> fCache;
};

class OperatorVoice : public Voice {
public:
    explicit OperatorVoice(float sampleRate);
    explicit OperatorVoice(BMessage* archive);
    virtual ~OperatorVoice();

    virtual status_t Archive(BMessage* into, bool deep = true) const override;
    static BArchivable* Instantiate(BMessage* archive);
    virtual const char* Name() override;
    virtual BView* CreateParamView() override;

    int32 AddOperator(Operator* op);
    Operator* OperatorAt(int32 index) const;
    int32 CountOperators() const;
    Operator* FindOperator(int32 id) const;
    status_t Connect(int32 sourceID, const char* outputName,
        int32 targetID, const char* inputName);
    status_t Connect(int32 sourceID, int32 sourceOutput,
        int32 targetID, int32 targetInput);
    void Disconnect(int32 targetID, const char* inputName);
    int32 NoteOperatorID() const;
    status_t SetAudioOutput(int32 sourceID, const char* outputName);
    status_t SetAudioOutput(int32 sourceID, int32 outputIndex);
    void ClearAudioOutput();
    bool HasAudioOutput() const;
    bool HasEnvelopes() const;

protected:
    virtual VoiceNote* createNote(uint8 note, float frequency, uint8 velocity,
        bigtime_t performanceTime) override;

private:
    friend class OperatorVoiceNote;
    status_t _Unarchive(const BMessage* archive);
    void _RefreshGraphIndices();
    void _SendNoteOff(OperatorVoiceNote& note, uint64 frame);

    BObjectList<Operator, true> fOperators;
    int32 fNextID;
    int32 fNoteOperatorID;
    OperatorOutput* fAudioOutput;
    OperatorValue fAudioDefault;
};

#endif // OPERATORVOICE_H
