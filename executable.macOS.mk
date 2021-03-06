#
# Executable
#
EXE_FILE = $(BIN_DIR)/Kowloon_Simulator_2020


#
# C Source Files
#
SRC_DIR = src
C_FILES = $(shell find $(SRC_DIR) -name "*.c")


#
# Object Files
#
# Create a list of C object files from our list of C source files.
O_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_FILES))


#
# Flags
#
# Target a reasonable C standard.
CFLAGS += -std=c11
# Enable more warning messages, and treat them as errors.
CFLAGS += -Wpedantic -Wall -Wmissing-prototypes -Wmissing-declarations -Wimplicit-fallthrough
CFLAGS += -Werror
# However, it's easy to trigger some warnings during development, so
# just let them be warnings.
CFLAGS += -Wno-extra-semi
CFLAGS += -Wno-error=unused-variable -Wno-error=unused-function -Wno-error=implicit-fallthrough
# TODO Declare DEBUG in Makefile
CFLAGS += -g3 -DDEBUG -DSTRICT
# Explicitly include the source directory.
CFLAGS += -I$(SRC_DIR)
CFLAGS += -I/usr/local/include
CFLAGS += $(shell sdl2-config --cflags)

LDFLAGS += -g
LDFLAGS += -framework OpenGL
LDFLAGS += -L/usr/local/lib
LDFLAGS += -lGLEW
LDFLAGS += $(shell sdl2-config --libs)


#
# Rules
#
.PHONY: exe
exe: $(EXE_FILE)

# Link compiled object files to form the executable
$(EXE_FILE): $(O_FILES)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Compile source code into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean_exe
clean_exe:
	-$(RM) $(O_FILES)
	-$(RM) $(EXE_FILE)
