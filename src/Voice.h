/*
 * Copyright 2026, YourName
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef VOICE_H
#define VOICE_H


#include <SupportDefs.h>
#include <ObjectList.h>
#include <View.h>

class Envelope {
public:
    bigtime_t           Attack = 0;
    bigtime_t           Decay  = 500000;
    float               Sustain = 1.0;
    bigtime_t           Release = 0;
};

class ParamControl;
class VoiceParams;

class VoiceParam {
    friend class ParamControl;
public:
    enum VoiceParamType {
        UINT7_TYPE,
        UINT8_TYPE,
        TIME_IN_USEC_TYPE,
        TIME_IN_FRAMES_TYPE,
        FLOAT_TYPE,
        GROUP_TYPE
    };
    enum VoiceParamDisplay {
        DISPLAY_KNOB,
        DISPLAY_SLIDER,
        DISPLAY_UP_DOWN
    };

    struct VoiceParamValue {
        union {
            uint8           uint8Value;
            bigtime_t       timeValue;
            float           floatValue;
            VoiceParams*    paramGroupValue;
        };
    };
                            VoiceParam(const char* name, VoiceParamType paramType,
                                    VoiceParamDisplay paramDisplay, uint8 value);
                            VoiceParam(const char* name, VoiceParamType paramType,
                                    VoiceParamDisplay paramDisplay, bigtime_t value);
                            VoiceParam(const char* name, VoiceParamType paramType,
                                    VoiceParamDisplay paramDisplay, float value,
                                    float minValue = -10.0, float maxValue = 10.0);
                            VoiceParam(const char* name, VoiceParamType paramType,
                                    VoiceParamDisplay paramDisplay, VoiceParams* value);

    virtual                 ~VoiceParam();
    void                    AddToLayout(BView* view);
    const char*             Name() { return fName; }
    VoiceParamType          Type() { return fType; }
    VoiceParamDisplay       Display() { return fDisplay; }
    VoiceParamValue         MinValue() { return fMinValue; }
    VoiceParamValue         MaxValue() { return fMaxValue; }
    VoiceParamValue         value;
protected:
    const char*             fName;
    VoiceParamType          fType;
    VoiceParamDisplay       fDisplay;
    VoiceParamValue         fMinValue;
    VoiceParamValue         fMaxValue;
};


class VoiceParams : public BObjectList<VoiceParam> { };

class Voice;

class VoiceNote {
friend class Voice;
public:
                            VoiceNote(Voice* voice, uint8 note, float frequency,
                                    uint8 velocity, bigtime_t performanceTime);
    virtual                 ~VoiceNote();
    virtual void            Recycle();
    virtual void            SetSampleRate(float sampleRate);
    virtual float           NextSample() = 0;
    virtual void            NoteOff(bigtime_t performanceTime);
protected:
    uint8                   fNote;
    bigtime_t               fNoteOnTime;
    bigtime_t               fNoteOffTime;
    enum voiceState {
        INACTIVE = 0,
        ACTIVE,
        RELEASE
    }                       fState;
    Voice*                  fVoice;
    float                   fVelocity;
    float                   fFrequency;
    float                   fSampleRate;
    float                   fdT;
    static const char*      fName;
    VoiceParams             fParams;
};

class VoiceNotes : public BObjectList<VoiceNote, true> { };

class Voice : public BArchivable {
    friend class VoiceNote;
public:
                            Voice(float SampleRate);
    virtual                 ~Voice();
    virtual VoiceNote*      NoteOn(uint8 note, float frequency, uint8 velocity, bigtime_t performanceTime);
    virtual bool            NoteOff(uint8 note, bigtime_t performanceTime);
    virtual void            Recycle(VoiceNote* voiceNote);
    virtual BObjectList<VoiceNote, true>*
                            ActiveNotes();
    virtual void            SetSampleRate(float sampleRate);
    virtual BView*          CreateParamView();
    virtual const char*     Name() = 0;
protected:
    virtual VoiceNote*      createNote(uint8 note, float frequency, uint8 velocity, bigtime_t performanceTime) = 0;
    float                   fSampleRate;
    VoiceNotes              fVoiceNotes;
    VoiceParams             fParams;
};




#endif // VOICE_H
