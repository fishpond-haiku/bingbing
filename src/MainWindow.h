/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <FilePanel.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>
#include "Synth.h"

class MainWindow : public BWindow
{
public:
							MainWindow(Synth* synth);
	virtual					~MainWindow();

	virtual void			MessageReceived(BMessage* msg);

    BView*                  VoiceParamView;
private:
    BMenuBar*		        _BuildMenu();
    BView*                  _BuildChannelView();

    status_t		        _LoadSettings(BMessage& settings);
    status_t		        _SaveSettings();

    BMenuItem*		        fSaveMenuItem;
    BFilePanel*		        fOpenPanel;
    BFilePanel*		        fSavePanel;

    Synth*                  fSynth;
};

#endif
