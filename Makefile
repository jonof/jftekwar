# TekWar Makefile for GNU Make

# Create Makefile.user yourself to provide your own overrides
# for configurable values
-include Makefile.user

##
##
## CONFIGURABLE OPTIONS
##
##

# Debugging options
RELEASE ?= 1

# Base path of app installation
PREFIX ?= /usr/local/share/games/jftekwar

# DirectX SDK location
DXROOT ?= $(USERPROFILE)/sdks/directx/dx81

# Engine source code path
EROOT ?= jfbuild

# Engine options
SUPERBUILD ?= 1
POLYMOST ?= 1
USE_OPENGL ?= 1
DYNAMIC_OPENGL ?= 1
NOASM ?= 0
LINKED_GTK ?= 0
WITHOUT_GTK ?= 1


##
##
## HERE BE DRAGONS
##
##

# build locations
SRC=src
RSRC=rsrc
EINC=$(EROOT)/include
ELIB=$(EROOT)
INC=$(SRC)
o=o

LIBSMACKERSRC=libsmacker

ifneq (0,$(RELEASE))
  # debugging disabled
  debug=-fomit-frame-pointer -O2
else
  # debugging enabled
  debug=-ggdb -O0 -Werror
endif

CC=gcc
CXX=g++
OURCFLAGS=$(debug) -W -Wall -Wimplicit -Wno-unused \
	-fno-pic -fno-strict-aliasing -DNO_GCC_BUILTINS \
	-I$(INC) -I$(EINC) -I$(LIBSMACKERSRC)
OURCXXFLAGS=-fno-exceptions -fno-rtti
LIBS=-lm -ldl
GAMELIBS=
NASMFLAGS=-s #-g
EXESUFFIX=

LIBSMACKEROBJ= \
	$(LIBSMACKERSRC)/smacker.$o \
	$(LIBSMACKERSRC)/smk_bitstream.$o \
	$(LIBSMACKERSRC)/smk_hufftree.$o

GAMEOBJS= \
	$(SRC)/b5compat.$o \
	$(SRC)/tekcdr.$o \
	$(SRC)/tekchng.$o \
	$(SRC)/tekgame.$o \
	$(SRC)/tekgun.$o \
	$(SRC)/tekldsv.$o \
	$(SRC)/tekmap.$o \
	$(SRC)/tekmsc.$o \
	$(SRC)/tekprep.$o \
	$(SRC)/teksmk.$o \
	$(SRC)/teksnd.$o \
	$(SRC)/tekspr.$o \
	$(SRC)/tekstat.$o \
	$(SRC)/tektag.$o \
	$(SRC)/tektxt.$o \
	$(SRC)/tekver.$o \
	$(LIBSMACKEROBJ)

EDITOROBJS=$(SRC)/bstub.$o

include $(EROOT)/Makefile.shared

ifeq ($(PLATFORM),LINUX)
	NASMFLAGS+= -f elf
endif
ifeq ($(PLATFORM),WINDOWS)
	OURCFLAGS+= -I$(DXROOT)/include
	NASMFLAGS+= -f win32 --prefix _
	GAMEOBJS+= $(SRC)/gameres.$o $(SRC)/startwin.game.$o
	EDITOROBJS+= $(SRC)/buildres.$o
endif

ifeq ($(RENDERTYPE),SDL)
	OURCFLAGS+= $(subst -Dmain=SDL_main,,$(shell sdl-config --cflags))

	ifeq (1,$(HAVE_GTK2))
		OURCFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
		GAMEOBJS+= $(SRC)/game_banner.$o $(SRC)/startgtk.game.$o
		EDITOROBJS+= $(SRC)/editor_banner.$o
	endif

	GAMEOBJS+= $(SRC)/game_icon.$o
	EDITOROBJS+= $(SRC)/build_icon.$o
endif

OURCFLAGS+= $(BUILDCFLAGS)

ifneq ($(PLATFORM),WINDOWS)
	OURCFLAGS+= -DPREFIX=\"$(PREFIX)\"
endif

.PHONY: clean all engine $(ELIB)/$(ENGINELIB) $(ELIB)/$(EDITORLIB)

# TARGETS

# Invoking Make from the terminal in OSX just chains the build on to xcode
ifeq ($(PLATFORM),DARWIN)
ifeq ($(RELEASE),0)
style=Debug
else
style=Release
endif
.PHONY: alldarwin
alldarwin:
	cd osx && xcodebuild -target All -buildstyle $(style)
endif

all: tekwar$(EXESUFFIX) build$(EXESUFFIX)

tekwar$(EXESUFFIX): $(GAMEOBJS) $(ELIB)/$(ENGINELIB)
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -o $@ $^ $(LIBS) $(GAMELIBS) -Wl,-Map=$@.map
	
build$(EXESUFFIX): $(EDITOROBJS) $(ELIB)/$(EDITORLIB) $(ELIB)/$(ENGINELIB)
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -o $@ $^ $(LIBS) -Wl,-Map=$@.map

include Makefile.deps

.PHONY: enginelib editorlib
enginelib editorlib:
	$(MAKE) -C $(EROOT) \
		SUPERBUILD=$(SUPERBUILD) POLYMOST=$(POLYMOST) \
		USE_OPENGL=$(USE_OPENGL) DYNAMIC_OPENGL=$(DYNAMIC_OPENGL) \
		WITHOUT_GTK=$(WITHOUT_GTK) NOASM=$(NOASM) RELEASE=$(RELEASE) $@

$(ELIB)/$(ENGINELIB): enginelib
$(ELIB)/$(EDITORLIB): editorlib

# RULES
$(SRC)/%.$o: $(SRC)/%.nasm
	nasm $(NASMFLAGS) $< -o $@

$(SRC)/%.$o: $(SRC)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@
$(SRC)/%.$o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@
$(LIBSMACKERSRC)/%.$o: $(LIBSMACKERSRC)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@

$(SRC)/%.$o: $(SRC)/misc/%.rc
	windres -i $< -o $@ --include-dir=$(EINC) --include-dir=$(SRC)

$(SRC)/%.$o: $(SRC)/util/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@

$(SRC)/%.$o: $(RSRC)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@

$(SRC)/game_banner.$o: $(RSRC)/game_banner.c
$(SRC)/editor_banner.$o: $(RSRC)/editor_banner.c
$(RSRC)/game_banner.c: $(RSRC)/game.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --rle --name=startbanner_pixdata $^ | sed '/pixel_data:/ a (guint8*)' >> $@
$(RSRC)/editor_banner.c: $(RSRC)/build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --rle --name=startbanner_pixdata $^ | sed '/pixel_data:/ a (guint8*)' >> $@

# PHONIES	
clean:
ifeq ($(PLATFORM),DARWIN)
	cd osx && xcodebuild -target All clean
else
	-rm -f $(GAMEOBJS) $(EDITOROBJS)
	$(MAKE) -C $(EROOT) clean
endif
	
veryclean: clean
ifeq ($(PLATFORM),DARWIN)
else
	-rm -f tekwar$(EXESUFFIX) build$(EXESUFFIX) core*
	$(MAKE) -C $(EROOT) veryclean
endif
