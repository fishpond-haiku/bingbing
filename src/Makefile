## Haiku Generic Makefile v2.6 ##

NAME = bingbing
TARGET_DIR = ../dist
TYPE = APP
APP_MIME_SIG = application/x-vnd.fishpond-bingbing

# Sources
SRCS = Voice.cpp \
       SineVoice.cpp \
       OperatorVoice.cpp \
       SynthOut.cpp \
       PanView.cpp \
       KnobControl.cpp \
       VoiceControls.cpp \
       Synth.cpp \
       MainWindow.cpp \
       SynthApp.cpp

RDEFS = Resources.rdef
RSRCS =

LIBS = be media midi2 tracker localestub $(STDCPPLIBS)
SYSTEM_INCLUDE_PATHS = $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/media) \
                       $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/interface)
LOCAL_INCLUDE_PATHS =
OPTIMIZE :=
LOCALES =
DEFINES = DEBUGGING
WARNINGS =
SYMBOLS :=
DEBUGGER :=
COMPILER_FLAGS =
LINKER_FLAGS =
DRIVER_PATH =

DEVEL_DIRECTORY := $(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
