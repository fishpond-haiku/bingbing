/*
 * Copyright 2024, Fishpond
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUGGING
#include <stdio.h>
#include <stdarg.h>
extern "C" {
	int _debugPrintf(const char *, ...);
	int _sPrintf(const char *, ...);
    int fprintf(FILE *stream, const char*, ...);
}

#define INFO( ARGS...) _debugPrintf( ARGS )
#define TRACE( ARGS...) fprintf(stderr, ARGS )

#else

#define TRACE( ARGS...)
#define INFO( ARGS...)

#endif // DEBUG

#endif // DEBUG_H
