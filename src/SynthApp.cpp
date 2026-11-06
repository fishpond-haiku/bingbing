/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <private/interface/AboutWindow.h>
#include "SynthApp.h"
#include "MainWindow.h"

#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"

const char* kApplicationSignature = "application/x-vnd.fishpond-bingbing";

SynthApp::SynthApp()
	:
	BApplication(kApplicationSignature)
{
	fSynth = new Synth();
	MainWindow* mainWindow = new MainWindow(fSynth);
	mainWindow->Show();
    fSynth->Register();
}


SynthApp::~SynthApp()
{
	fSynth->Unregister();
}


void
SynthApp::AboutRequested()
{
	BAboutWindow* about
		= new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("bingbing"), kApplicationSignature);
	about->AddDescription(B_TRANSLATE("Makes noise on midi input."));
	about->AddCopyright(2024, "Fishpond <kai.niessen@online.de>");
	about->Show();
}

void
SynthApp::Quit()
{
	fSynth->Release();
	BApplication::Quit();
}

int
main()
{
	SynthApp app;
	app.Run();
	return 0;
}
