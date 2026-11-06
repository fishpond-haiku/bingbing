/*
 * Copyright 2026, YourName
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef KNOBCONTROL_H
#define KNOBCONTROL_H

#include <GraphicsDefs.h>
#include <Control.h>
#include <Slider.h>
#include <Picture.h>
#include <SupportDefs.h>


class KnobControl : public BControl {
public:
                                KnobControl(const char* name, const char* label,
                                        BMessage* message,
                                        uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FOLLOW_LEFT_TOP,
                                        float value = 0.0,
                                        float minValue = 0.0, float maxValue = 100.0);
    virtual                     ~KnobControl();
    virtual float               Value();
    virtual void                SetValue(int32 value) override;
    virtual void                SetValue(float value);
    virtual void                AttachedToWindow() override;
    virtual void				MessageReceived(BMessage* message) override;
	virtual	void				KeyDown(const char* bytes, int32 numBytes) override;
	virtual	void				MouseDown(BPoint where) override;
	virtual	void				MouseUp(BPoint where) override;
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage) override;
	virtual	void				GetPreferredSize(float* _width,
									float* _height) override;

    enum                        KnobColors {
                                    KNOB_COLOR_LINE,
                                    KNOB_COLOR_FRONT,
                                    KNOB_COLOR_SIDE,
                                    KNOB_COLOR_LEVEL,
                                    KNOB_COLOR_HOVER,
                                    KNOB_COLOR_MARK
                                };

protected:
    void                        Draw(BRect updateRect) override;

private:
    rgb_color                   fColors[KNOB_COLOR_MARK + 1];
    font_height                 fFontHeight;
    bool                        fIsMouseOver;
    bool                        fIsDragging;
    float                       fMouseY;
    float                       fTotalFontHeight;
    BPicture                    fKnobShape;
    float                       fMinValue;
    float                       fMaxValue;
    float                       fValue;
    float                       fAngle;
    bool                        fFine;
    static rgb_color            kColors[KNOB_COLOR_MARK + 1];

};

#endif // KNOBCONTROL_H
