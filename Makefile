# Note to self: clang is slow at compiling and produces slower asm than gcc, at least for this project.

# Compiler and flags
CC := clang
CFLAGS := -g -Wall -O0 -fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope  -std=c11 -Iinclude -Ilua/include
#CFLAGS := -g -Wall -O3 -std=c11 -Iinclude -Ilua/include
LDFLAGS := -Llua/lib -llua54

# Directories
SRC_DIR := src
INC_DIR := include
BIN_DIR := build

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(SRCS))

# Targets
ifeq ($(OS),Windows_NT)
	EXECUTABLE := $(BIN_DIR)/cslw.exe
	RM := del /Q /F
    MKDIR := mkdir
    CV2PDB := cv2pdb
else
	EXECUTABLE := $(BIN_DIR)/cslw
	RM := rm -rf
	MKDIR := mkdir -p
endif

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
#ifeq ($(OS),Windows_NT)
#	$(if $(CV2PDB),$(CV2PDB) $@ $@ $@.pdb)
#endif

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/cslw/cslw.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	$(MKDIR) $(BIN_DIR)

clean:
	$(RM) $(BIN_DIR)