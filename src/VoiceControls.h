/*
 * Copyright 2026, Fishpond
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef VOICECONTROLS_H
#define VOICECONTROLS_H


#include <SupportDefs.h>
#include <Slider.h>
#include <StringView.h>
#include "KnobControl.h"
#include "Voice.h"


class ParamControl {
public:
                        ParamControl(VoiceParam* param, BStringView* valueLabel = NULL,
                                const char* format = "%f");
    void                SetValue(int32 value);
    void                SetValue(float value);
protected:
    VoiceParam*         fTarget;
    BStringView*        fValueLabel;
    const char*         fFormat;
};

class SliderFloat : public BSlider, public ParamControl {
public:
                        SliderFloat(VoiceParam* param, BStringView* valueLabel = NULL,
                                const char* format = "%f");
    void                SetValue(int32 value) override;
private:
    BString             fLabel;
    BString             fMinLabel;
    BString             fMaxLabel;
};

class KnobFloat : public KnobControl, public ParamControl {
public:
                        KnobFloat(VoiceParam* param, BStringView* valueLabel = NULL,
                                const char* format = "%f");
    void                SetValue(float value) override;
private:
    BString             fLabel;
};

#endif // VOICECONTROLS_H
