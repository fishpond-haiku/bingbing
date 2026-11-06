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
#include <View.h>

#include <memory>
#include <vector>

#include "Voice.h"

class OperatorVoiceNote;

struct OperatorValue {
    enum Type {
        FLOAT_VALUE,
        EVENT_VALUE,
        BOOL_VALUE,
        SELECTOR_VALUE,
        TIME_VALUE
    } type;
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

struct OperatorPort {
    const char* name;
    OperatorValue::Type type;
};

class OperatorState {
public:
    virtual ~OperatorState() { }
};

class Operator : public BArchivable {
public:
    virtual ~Operator();

    virtual const char* ClassName() const = 0;
    virtual std::unique_ptr<OperatorState> CreateState() const = 0;
    virtual OperatorValue EvaluateOutput(int32 output, OperatorVoiceNote& note,
        OperatorState& state) const = 0;
    virtual void ReceiveEvent(int32 input, OperatorVoiceNote& note,
        OperatorState& state, uint64 frame);

    int32 InputCount() const;
    int32 OutputCount() const;
    const OperatorPort& Input(int32 index) const;
    const OperatorPort& Output(int32 index) const;
    const OperatorValue& InputDefault(int32 index) const;
    status_t SetInputDefault(int32 index, const OperatorValue& value);
    BPoint Position() const;
    void SetPosition(BPoint position);

    virtual status_t Archive(BMessage* into, bool deep = true) const override;
    status_t Unarchive(const BMessage* archive);

protected:
    Operator(const OperatorPort* inputs, int32 inputCount,
        const OperatorPort* outputs, int32 outputCount);
    explicit Operator(BMessage* archive);

private:
    std::vector<OperatorPort> fInputs;
    std::vector<OperatorPort> fOutputs;
    std::vector<OperatorValue> fDefaults;
    BPoint fPosition;
};

class NoteOperator : public Operator {
public:
    NoteOperator();
    explicit NoteOperator(BMessage* archive);
    virtual const char* ClassName() const override;
    virtual std::unique_ptr<OperatorState> CreateState() const override;
    virtual OperatorValue EvaluateOutput(int32 output, OperatorVoiceNote& note,
        OperatorState& state) const override;
};

class SineOperator : public Operator {
public:
    SineOperator();
    explicit SineOperator(BMessage* archive);
    virtual const char* ClassName() const override;
    virtual std::unique_ptr<OperatorState> CreateState() const override;
    virtual OperatorValue EvaluateOutput(int32 output, OperatorVoiceNote& note,
        OperatorState& state) const override;
};

class EnvelopeOperator : public Operator {
public:
    EnvelopeOperator();
    explicit EnvelopeOperator(BMessage* archive);
    virtual const char* ClassName() const override;
    virtual std::unique_ptr<OperatorState> CreateState() const override;
    virtual OperatorValue EvaluateOutput(int32 output, OperatorVoiceNote& note,
        OperatorState& state) const override;
    virtual void ReceiveEvent(int32 input, OperatorVoiceNote& note,
        OperatorState& state, uint64 frame) override;
    bool IsFinished(const OperatorState& state) const;
};

class NoiseOperator : public Operator {
public:
    NoiseOperator();
    explicit NoiseOperator(BMessage* archive);
    virtual const char* ClassName() const override;
    virtual std::unique_ptr<OperatorState> CreateState() const override;
    virtual OperatorValue EvaluateOutput(int32 output, OperatorVoiceNote& note,
        OperatorState& state) const override;
};

class OperatorVoiceNote : public VoiceNote {
public:
    OperatorVoiceNote(class OperatorVoice* voice, uint8 note, float frequency,
        uint8 velocity, bigtime_t performanceTime);
    virtual float NextSample() override;
    virtual void NoteOff(bigtime_t performanceTime) override;

    OperatorValue InputValue(const Operator* op, int32 input);
    OperatorValue OutputValue(const Operator* op, int32 output);
    uint64 Frame() const;
    float SampleRate() const;
    float NoteFrequency() const;
    float NoteVelocity() const;
    class OperatorVoice* Graph() const;

private:
    struct CachedOutput {
        uint64 frame;
        bool evaluating;
        bool valid;
        OperatorValue value;
        CachedOutput() : frame(0), evaluating(false), valid(false) { }
    };
    class OperatorVoice* fGraph;
    uint64 fFrame;
    std::vector<std::unique_ptr<OperatorState> > fStates;
    std::vector<std::vector<CachedOutput> > fCache;
};

class OperatorVoice : public Voice {
public:
    struct Connection {
        int32 sourceID;
        int32 sourceOutput;
        int32 targetID;
        int32 targetInput;
    };

    explicit OperatorVoice(float sampleRate);
    explicit OperatorVoice(BMessage* archive);
    virtual ~OperatorVoice();

    virtual status_t Archive(BMessage* into, bool deep = true) const override;
    static BArchivable* Instantiate(BMessage* archive);
    virtual const char* Name() override;

    int32 AddOperator(Operator* op);
    Operator* OperatorAt(int32 index) const;
    int32 CountOperators() const;
    Operator* FindOperator(int32 id) const;
    int32 IDForOperator(const Operator* op) const;
    status_t Connect(int32 sourceID, int32 sourceOutput,
        int32 targetID, int32 targetInput);
    void Disconnect(int32 targetID, int32 targetInput);
    int32 NoteOperatorID() const;
    status_t SetAudioOutput(int32 sourceID, int32 sourceOutput);
    void ClearAudioOutput();
    bool HasAudioOutput() const;
    bool HasEnvelopes() const;
    const std::vector<Connection>& Connections() const;

protected:
    virtual VoiceNote* createNote(uint8 note, float frequency, uint8 velocity,
        bigtime_t performanceTime) override;

private:
    friend class OperatorVoiceNote;
    int32 _IndexForID(int32 id) const;
    OperatorValue _InputValue(OperatorVoiceNote& note, const Operator* op,
        int32 input);
    OperatorValue _OutputValue(OperatorVoiceNote& note, const Operator* op,
        int32 output);
    void _DispatchNoteOff(OperatorVoiceNote& note, uint64 frame);
    status_t _Unarchive(const BMessage* archive);

    struct Entry {
        int32 id;
        std::unique_ptr<Operator> op;
        Entry(int32 operatorID, Operator* object) : id(operatorID), op(object) { }
    };
    std::vector<Entry> fOperators;
    std::vector<Connection> fConnections;
    int32 fNextID;
    int32 fNoteOperatorID;
    int32 fAudioSourceID;
    int32 fAudioSourceOutput;
    OperatorValue fAudioDefault;
};

#endif // OPERATORVOICE_H
