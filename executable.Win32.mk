#
# Executable
#
EXE_FILE = $(BIN_DIR)\Cyberpunk1997.exe


#
# C Source Files
#
SRC_DIR = src
C_FILES = $(patsubst $(shell cd)\\%,%,$(shell dir /a /b /s "$(SRC_DIR)\*.c"))


#
# Object Files
#
O_FILES = $(patsubst $(SRC_DIR)\\%.c,$(BUILD_DIR)\\%.obj,$(C_FILES))


#
# Flags
#
CFLAGS += /std:c11
CFLAGS += /I$(SRC_DIR)
CFLAGS += /Ilib\glew-2.1.0\include
CFLAGS += /Ilib\SDL2-2.0.12\include

LDFLAGS += /LIBPATH:lib\glew-2.1.0\lib\Release\Win32
LDFLAGS += /LIBPATH:lib\SDL2-2.0.12\lib\x86
LDFLAGS += /SUBSYSTEM:WINDOWS
LDFLAGS += /DEBUG:FULL

LDLIBS += shell32.lib
LDLIBS += opengl32.lib
LDLIBS += glew32.lib
LDLIBS += SDL2main.lib
LDLIBS += SDL2.lib


#
# Rules
#
.PHONY: exe
exe: $(EXE_FILE)

$(EXE_FILE): $(O_FILES)
	if not exist $(@D) mkdir $(@D)
	xcopy lib\glew-2.1.0\bin\Release\Win32\glew32.dll /D $(BIN_DIR)
	xcopy lib\SDL2-2.0.12\lib\x86\SDL2.dll /D $(BIN_DIR)
	link.exe $(LDFLAGS) $^ $(LDLIBS) /OUT:$@

$(BUILD_DIR)\\%.obj: $(SRC_DIR)\%.c
	if not exist $(@D) mkdir $(@D)
	cl.exe $(CFLAGS) /c /Fo$@ $<

.PHONY: clean_exe
clean_exe:
	del $(O_FILES)
	del $(EXE_FILE)
