#
# Platform detection
#
ifeq ($(OS),Windows_NT)
	PLATFORM := Win32
else ifeq ($(shell uname -s),Darwin)
	PLATFORM := macOS
else
	$(error Unsupported platform)
endif

#
# Top level declarations
#
# Any declarations common to multiple makefiles should go here.
BIN_DIR = bin
BUILD_DIR = build
# TODO Declare DEBUG here


#
# Include other makefiles
#
# Seperating makefiles by concern is nice, but can result in variable
# name collisions or other bugs. The best defense is to be mindful
# while editing makefiles.

# The executable makefile provides `executable` and `clean_executable`
# as targets, and `EXECUTABLE_FILE` as a variable
include executable.mk

# The asset makefile provides `assets` as a target and `ASSET_FILES`
# as a variable
include assets.mk


#
# Targets
#
.PHONY: all
all: exe assets

.PHONY: clean
clean: clean_exe clean_assets

.PHONY: run
run: all
	$(EXE_FILE)
