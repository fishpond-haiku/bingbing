/*
 * Copyright 2026, Kai Niessen
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "PanView.h"

PanView::PanView(Synth* synth)
    :
    BView("panView", B_WILL_DRAW | B_FOLLOW_ALL_SIDES | B_FRAME_EVENTS),
    fPosition(0.5, 0.5),
    fSynth(synth),
    fMouseDown(false)
{ }

PanView::~PanView()
{ }

void
PanView::Draw(BRect updateRect) {
    BRect f = Bounds();
    this->SetHighColor(30, 150, 10);
    this->FillRect(f);
    this->SetHighColor(220, 20, 0);
    BPoint position = BPoint(fPosition.x * f.Width(), fPosition.y * f.Height());
    this->FillEllipse(BRect(position - BPoint(5,5), position + BPoint(5,5)));

    SetHighColor(0, 0, 0);
    switch (fSynth->OutputFormat()->channel_count) {
        case 2:
        {
            this->FillRoundRect(BRect(5, 5, 25, 25), 5, 5);
            this->FillRoundRect(BRect(f.right - 25, 5, f.right - 5, 25), 5, 5);
        } break;

        case 6:
        {
            this->FillRoundRect(BRect(5, 5, 25, 25), 5, 5);
            this->FillRoundRect(BRect(f.right - 25, 5, f.right - 5, 25), 5, 5);

            this->FillRoundRect(BRect(f.right / 2 - 10, 25, f.right / 2 + 10, 45), 5, 5);

            this->FillRoundRect(BRect(5, f.bottom - 25, 25, f.bottom - 5), 5, 5);
            this->FillRoundRect(BRect(f.right - 25, f.bottom - 25, f.right - 5, f.bottom - 5), 5, 5);
        } break;
    }
}

void
PanView::FrameResized(float width, float height) {
    Invalidate();
}

void
PanView::MouseDown(BPoint point) {
    fMouseDown = true;
    SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
    BRect f = Bounds();
    positionChange(BPoint(point.x / f.Width(), point.y / f.Height()));
}

void
PanView::MouseMoved(BPoint point, uint32 transit, const BMessage* message) {
    if (fMouseDown && transit == B_INSIDE_VIEW) {
        BRect f = Bounds();
        positionChange(BPoint(point.x / f.Width(), point.y / f.Height()));
    }
}

void
PanView::MouseUp(BPoint point) {
    BRect f = Bounds();
    positionChange(BPoint(point.x / f.Width(), point.y / f.Height()), true);
    SetMouseEventMask(B_POINTER_EVENTS, 0);
    fMouseDown = false;
}

void
PanView::positionChange(BPoint to, bool forceUpdate) {
    if (fPrevPosition == to)
        return;

    fPosition = to;
    to -= fPrevPosition;
    if (forceUpdate || to.x * to.x + to.y * to.y > 0.004) {
        fSynth->SetPan(fPosition);
        fPrevPosition = fPosition;
    }
    Invalidate();
}