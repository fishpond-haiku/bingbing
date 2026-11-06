/*
 * Copyright 2026, Fishpond
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "VoiceControls.h"
#include "Debug.h"

#define MIN_FLOAT_INT32 0
#define MAX_FLOAT_INT32 1 << 31 - 1


ParamControl::ParamControl(VoiceParam* param, BStringView* valueLabel, const char* format)
    :
    fTarget(param),
    fValueLabel(valueLabel),
    fFormat(format)
{
    if (fValueLabel) {
        BString fLabel;
        fLabel.SetToFormat(fFormat, param->value.floatValue);
        fValueLabel->SetText(fLabel);
    }
}

void
ParamControl::SetValue(int32 value) {
    switch (fTarget->Type()) {
        case VoiceParam::FLOAT_TYPE: {
            fTarget->value.floatValue = fTarget->fMinValue.floatValue +
                (value - MIN_FLOAT_INT32) / (float)(MAX_FLOAT_INT32 - MIN_FLOAT_INT32) *
                (fTarget->fMaxValue.floatValue - fTarget->fMinValue.floatValue);
            break;
        }
        default: {
            TRACE("ParamControl::SetValue: Unknown param type %d\n", fTarget->Type());
        }
    }
}

void
ParamControl::SetValue(float value) {
    BString fLabel;
    switch (fTarget->Type()) {
        case VoiceParam::FLOAT_TYPE:
            fTarget->value.floatValue = value;
            if (fValueLabel) {
                fLabel.SetToFormat(fFormat, value);
                fValueLabel->SetText(fLabel);
            }
            break;
        default:
            TRACE("ParamControl::SetValue: Bad param type %d for float value\n", fTarget->Type());
    }
}

SliderFloat::SliderFloat(VoiceParam* param, BStringView* valueLabel, const char* format)
    :
    ParamControl(param, valueLabel, format),
    BSlider(param->Name(), param->Name(), NULL, MIN_FLOAT_INT32, MAX_FLOAT_INT32, B_VERTICAL)
{
    float minValue = param->MinValue().floatValue;
    float maxValue = param->MaxValue().floatValue;
    BSlider::SetValue(MIN_FLOAT_INT32 + (param->value.floatValue - minValue) /
            (maxValue - minValue) *
            ((MAX_FLOAT_INT32) - MIN_FLOAT_INT32));
    fMinLabel.SetToFormat("%f", minValue);
    fMaxLabel.SetToFormat("%f", maxValue);

    BSlider::SetLimitLabels(fMinLabel, fMaxLabel);
}

void
SliderFloat::SetValue(int32 value) {
    BSlider::SetValue(value);
    ParamControl::SetValue(Value());
}

KnobFloat::KnobFloat(VoiceParam* param, BStringView* valueLabel, const char* format)
    :
    ParamControl(param, valueLabel, format),
    KnobControl(param->Name(), param->Name(), NULL, B_NAVIGABLE | B_WILL_DRAW | B_FOLLOW_LEFT_TOP,
            param->value.floatValue, param->MinValue().floatValue, param->MaxValue().floatValue)
{ }

void
KnobFloat::SetValue(float value) {
    KnobControl::SetValue(value);
    ParamControl::SetValue(Value());
}



