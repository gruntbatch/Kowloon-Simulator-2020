BIN_DIR = bin
BUILD_DIR = build
SRC_DIR = src
EXE_FILE = $(BIN_DIR)/a.out

C_FILES = $(shell find $(SRC_DIR) -name "*.c")
O_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_FILES))

CFLAGS += -std=c11
CFLAGS += -Wall -Wmissing-declarations -Wmissing-prototypes -Wpedantic
CFLAGS += -Werror
CFLAGS += -Wno-extra-semi
CFLAGS += -Wno-error=unused-variable -Wno-error=unused-function
CFLAGS += -g3
CFLAGS += -I$(SRC_DIR)

LDFLAGS += -g

LDFLAGS += -framework OpenGL
CFLAGS += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs)

.PHONY: exe
exe: $(EXE_FILE)

$(EXE_FILE): $(O_FILES)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	-$(RM) $(O_FILES)
	-$(RM) $(EXE_FILE)

run: exe
	$(EXE_FILE)
