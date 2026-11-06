/*
 * Copyright 2026, Fishpond
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PANVIEW_H
#define PANVIEW_H


#include <SupportDefs.h>
#include <View.h>
#include "Synth.h"


class PanView : public BView {
public:
                            PanView(Synth* synth);
                            ~PanView();
    virtual void            Draw(BRect updateRect) override;
    virtual void            FrameResized(float width, float height) override;
    virtual void            MouseDown(BPoint point) override;
    virtual void            MouseMoved(BPoint point, uint32 transit, const BMessage* message) override;
    virtual void            MouseUp(BPoint point) override;

private:
    void                    positionChange(BPoint to, bool forceUpdate = false);
    Synth*                  fSynth;
    BPoint                  fPosition;
    BPoint                  fPrevPosition;
    bool                    fMouseDown;
};

#endif // PANVIEW_H
