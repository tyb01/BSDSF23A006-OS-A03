# Compiler and flags
CC       := gcc
CFLAGS   := -Wall -Wextra -g -Iinclude
LDFLAGS := -lreadline

# Directories
SRC_DIR  := src
OBJ_DIR  := build
BIN_DIR  := bin

# Target binary
TARGET   := $(BIN_DIR)/myshell

# Source and object files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Default target
all: $(TARGET)

# Linking step
$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	@echo " Linking..."
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# Compilation rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo " Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

# Directory creation
$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

# Run the shell directly
run: $(TARGET)
	@echo " Running myshell..."
	@./$(TARGET)

# Clean up build artifacts
clean:
	@echo " Cleaning up..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo " Done."

# For rebuilding everything from scratch
rebuild: clean all

# Phony targets
.PHONY: all clean run rebuild
