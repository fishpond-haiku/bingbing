/*
 * Copyright 2026, Fishpond
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <math.h>
#include <IconUtils.h>
#include <InterfaceDefs.h>
#include "KnobControl.h"

KnobControl::KnobControl(const char* name, const char* label, BMessage* message, uint32 flags,
        float value, float minValue, float maxValue)
    :
    BControl(name, label, message, flags | B_WILL_DRAW),
    fValue(value),
    fMinValue(minValue),
    fMaxValue(maxValue),
    fAngle(270.0 * value / (maxValue - minValue)),
    fIsMouseOver(false),
    fIsDragging(false),
    fFine(false)
{
    for (int i = 0; i <= KnobColors::KNOB_COLOR_MARK; i++)
        fColors[i] = kColors[i];
    SetValueNoUpdate(value / (maxValue - minValue) * 100);
}


KnobControl::~KnobControl() {}

float
KnobControl::Value() {
    return fValue;

}

void
KnobControl::SetValue(int32 value) {
    BControl::SetValue(value);
    fValue = fMinValue + (fMaxValue - fMinValue) * value / 100.0;
    if (fValue < fMinValue)
        fValue = fMinValue;
    else if (fValue > fMaxValue)
        fValue = fMaxValue;
    fAngle = fValue / (fMaxValue - fMinValue) * 270.0;
    Invalidate();
}

void
KnobControl::SetValue(float value) {
    if (value < fMinValue)
        fValue = fMinValue;
    else if (value > fMaxValue)
        fValue = fMaxValue;
    else
        fValue = value;
    fAngle = fValue / (fMaxValue - fMinValue) * 270.0;
    BControl::SetValue((value - fMinValue) / (fMaxValue - fMinValue) * 100.0);
    Invalidate();
}

void
KnobControl::AttachedToWindow() {
    GetFontHeight(&fFontHeight);
    fTotalFontHeight = fFontHeight.ascent + fFontHeight.leading + fFontHeight.descent;
    BControl::AttachedToWindow();
}

void
KnobControl::MessageReceived(BMessage* message) {
    switch (message->what) {
        case B_MOUSE_WHEEL_CHANGED: {
            float delta = message->FindFloat("be:wheel_delta_y");
            float factor = (modifiers() & B_SHIFT_KEY) ? 100.0F : 1000.0F;
            SetValue(fValue - delta / factor * (fMaxValue - fMinValue));
            break;
        }
        default:
            BControl::MessageReceived(message);
    }
}

void
KnobControl::KeyDown(const char* bytes, int32 numBytes) {
    if (numBytes == 1) {
        switch (*bytes) {
            case B_LEFT_ARROW:
            case B_DOWN_ARROW: {
                float factor = (modifiers() & B_SHIFT_KEY) ? 100.0F : 1000.0F;
                SetValue((float)Value() - (fMaxValue - fMinValue) / factor);
                break;
            }
            case B_RIGHT_ARROW:
            case B_UP_ARROW: {
                float factor = (modifiers() & B_SHIFT_KEY) ? 100.0F : 1000.0F;
                SetValue((float)Value() + (fMaxValue - fMinValue) / factor);
                break;
            }
        }
    }
    BControl::KeyDown(bytes, numBytes);
}

void
KnobControl::MouseDown(BPoint where) {
    if (!IsEnabled()) return;
    MakeFocus(true);
    fIsDragging = true;
    SetMouseEventMask(B_LOCK_WINDOW_FOCUS);
    fMouseY = where.y;
}

void
KnobControl::MouseUp(BPoint where) {
    SetMouseEventMask(0);
    fIsDragging = false;
}

void
KnobControl::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage) {
    if (!IsEnabled()) return;
    if (fIsDragging) {
        float factor = (modifiers() & B_SHIFT_KEY)? 60.0F : 6.0F;
        SetValue(fValue - (where.y - fMouseY) / factor / fTotalFontHeight * (fMaxValue - fMinValue));
        fMouseY = where.y;
    } else
        switch (code) {
            case B_ENTERED_VIEW:
                fIsMouseOver = true;
                Invalidate();
                break;
            case B_EXITED_VIEW:
                fIsMouseOver = false;
                Invalidate();
                break;
        }
}

void
KnobControl::GetPreferredSize(float* _width, float* _height) {
    *_width = std::max<float>(std::max<float>(*_width, 4.0 * fTotalFontHeight), StringWidth(Label()));
    *_height = 4.5 * fTotalFontHeight;
}

void
KnobControl::Draw(BRect updateRect) {
    BRect frame = Bounds();

    // Erase background
    SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    FillRect(frame);

    float centerX = frame.Width() / 2.0;
    BRect area(centerX - 2.0 * fTotalFontHeight, 0, centerX + 2.0 * fTotalFontHeight, 4.0 * fTotalFontHeight);

    // Draw Label
    float labelWidth = StringWidth(Label());
    SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
    DrawString(Label(), BPoint(centerX - labelWidth / 2, area.top + 4.5 * fTotalFontHeight - fFontHeight.descent));

    // Draw Level
    SetHighColor(fColors[fIsMouseOver ? KNOB_COLOR_HOVER : KNOB_COLOR_LEVEL]);
    SetPenSize(fTotalFontHeight * 0.6);
    area.InsetBy(fTotalFontHeight * 0.3, fTotalFontHeight * 0.3);
    StrokeArc(area, 225 - fAngle, fAngle);

    // Draw Knob base
    area.InsetBy(8, 8);
    SetPenSize(1.0);
    SetHighColor(fColors[KNOB_COLOR_SIDE]);
    FillEllipse(area);
    SetHighColor(IsFocus() ? ui_color(B_CONTROL_MARK_COLOR) : fColors[KNOB_COLOR_LINE]);
    StrokeEllipse(area);

    area.InsetBy(4, 4);
    SetHighColor(fIsMouseOver ? tint_color(fColors[KNOB_COLOR_FRONT], B_LIGHTEN_1_TINT) :
            fColors[KNOB_COLOR_FRONT]);
    FillEllipse(area);
    SetHighColor(fColors[KNOB_COLOR_LINE]);
    StrokeEllipse(area);

    BPoint markCenter(
        area.left + area.Width() / 2.0 + (area.Width() / 2.0 - 6.0) * cos((225 - fAngle) / 180.0 * M_PI),
        area.top + area.Height() / 2.0 - (area.Height() / 2.0 - 6.0) * sin((225 - fAngle) / 180.0 * M_PI));

    area = BRect(markCenter, markCenter);
    area.InsetBy(-3.0, -3.0);
    SetHighColor(IsEnabled() ? fColors[KNOB_COLOR_MARK] :
            tint_color(fColors[KNOB_COLOR_FRONT], B_DARKEN_2_TINT));
    FillEllipse(area);
}

rgb_color
KnobControl::kColors[] = {
    ui_color(B_CONTROL_BORDER_COLOR),       // KNOB_COLOR_LINE,
    tint_color(ui_color(B_CONTROL_BACKGROUND_COLOR), B_LIGHTEN_1_TINT),
                                            // KNOB_COLOR_FRONT,
    ui_color(B_CONTROL_BACKGROUND_COLOR),   // KNOB_COLOR_SIDE,
    { 0xD0, 0x20, 0x20, 0xff },             // KNOB_COLOR_LEVEL
    { 0xff, 0x60, 0x20, 0xff },             // KNOB_COLOR_HOVER
    { 0xf0, 0x30, 0x00, 0xff }              // KNOB_COLOR_MARK
};