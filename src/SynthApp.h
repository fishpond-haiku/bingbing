/*
 * Copyright 2024, My Name 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef APP_H
#define APP_H


#include <Application.h>
#include "Synth.h"

class SynthApp : public BApplication
{
public:
							SynthApp();
	virtual					~SynthApp();
	virtual void			AboutRequested();
	virtual void 			Quit();
	
private:
	Synth*					fSynth;
};

#endif // APP_H

