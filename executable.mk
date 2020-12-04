SRC_DIR = src


# Compilation is _very_ platform-specific, so there's not really a
# whole lot to do before including playform-specific stuff
include executable.$(PLATFORM).mk
