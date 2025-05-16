# Compiler
CC = clang++

# Compiler flags
# -g and -O0 are for debugging
CFLAGS = -std=c++11 -Wall -Wextra \
	-I./src -I./src/lib \
  -Wno-unused-function \
	-Wno-unused-parameter \
	-Wno-unused-variable \
	-g -O0

# Directories
SRCDIR = ./src
OBJDIR = ./build

# Base source files (utility and ATRAC files)
define BASE_SRC
$(wildcard $(SRCDIR)/io/*.cpp) \
$(wildcard $(SRCDIR)/util/*.cpp) \
$(wildcard $(SRCDIR)/audio/*.cpp) \
$(wildcard $(SRCDIR)/atrac/*.cpp)
endef

# Target-specific source files
DECODER_SRC = $(SRCDIR)/main_decoder.cpp $(BASE_SRC)
DECODER_OBJ = $(DECODER_SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DECODER_TARGET = decoder

TEST_SRC = $(SRCDIR)/main_test.cpp $(BASE_SRC) $(wildcard $(SRCDIR)/tests/*.cpp)
TEST_OBJ = $(TEST_SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TEST_TARGET = test

# Default target
all: $(DECODER_TARGET) $(TEST_TARGET)

# Build rules
decoder: $(DECODER_OBJ)
	@echo "Linking $(DECODER_TARGET) ..."
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_OBJ)
	@echo "Linking $(TEST_TARGET) ..."
	$(CC) $(CFLAGS) -o $@ $^

# Rule to compile source files into object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -rf $(OBJDIR) $(DECODER_TARGET) $(TEST_TARGET)

# Phony targets
.PHONY: all clean test
