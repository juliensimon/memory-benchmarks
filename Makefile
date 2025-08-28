# Memory Bandwidth Test Tool Makefile - Refactored Structure
#
# This Makefile builds the memory benchmark tool with platform-specific implementations

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -mtune=native -pthread -Wall -Wextra
LDFLAGS = -pthread

# Target executable
TARGET = memory_bandwidth

# Directory structure
COMMON_DIR = common
PLATFORM_DIR = platforms

# Common source files
COMMON_SOURCES = $(COMMON_DIR)/test_patterns.cpp \
                $(COMMON_DIR)/standard_tests.cpp \
                $(COMMON_DIR)/output_formatter.cpp \
                $(COMMON_DIR)/output_formatter_utils.cpp \
                $(COMMON_DIR)/working_sets.cpp \
                $(COMMON_DIR)/platform_factory.cpp \
                $(COMMON_DIR)/argument_parser.cpp \
                $(COMMON_DIR)/system_info_display.cpp \
                $(COMMON_DIR)/memory_utils.cpp

# Main source
MAIN_SOURCE = main.cpp

# Platform-specific sources (auto-detected)
PLATFORM_SOURCES = 
ifeq ($(shell uname),Darwin)
    # macOS
    PLATFORM_SOURCES += $(PLATFORM_DIR)/macos/macos_platform.cpp
    CXXFLAGS += -DPLATFORM_MACOS
else ifeq ($(shell uname),Linux)
    # Linux - detect architecture
    ARCH := $(shell uname -m)
    ifeq ($(ARCH),x86_64)
        PLATFORM_SOURCES += $(PLATFORM_DIR)/intel_x64/intel_platform.cpp
        CXXFLAGS += -DPLATFORM_INTEL_X64
    else ifeq ($(ARCH),aarch64)
        PLATFORM_SOURCES += $(PLATFORM_DIR)/arm64/arm64_platform.cpp
        CXXFLAGS += -DPLATFORM_ARM64
    endif
endif

# All sources
SOURCES = $(MAIN_SOURCE) $(COMMON_SOURCES) $(PLATFORM_SOURCES)

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Include directories
INCLUDES = -I.

# Default target
all: $(TARGET)

# Compile the program
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Compile object files
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET) $(OBJECTS)
	@echo "Clean complete"

# Install dependencies (cross-platform)
install-deps:
	@echo "Installing build dependencies..."
	@if [ "$(shell uname)" = "Darwin" ]; then \
		echo "macOS detected - assuming gcc/g++ are available"; \
		echo "If build fails, please install gcc via: brew install gcc"; \
	elif [ -f /etc/debian_version ]; then \
		sudo apt-get update; \
		sudo apt-get install -y build-essential; \
	elif [ -f /etc/redhat-release ]; then \
		sudo yum groupinstall -y "Development Tools" || sudo dnf groupinstall -y "Development Tools"; \
	else \
		echo "Unknown OS - please ensure you have a C++17 compiler installed"; \
	fi
	@echo "Dependencies check complete"

# Show build information
info:
	@echo "Memory Bandwidth Test Tool Build Information:"
	@echo "  Platform: $(shell uname) $(shell uname -m)"
	@echo "  Compiler: $(CXX)"
	@echo "  C++ Standard: C++17"
	@echo "  Optimization: -O3"
	@echo "  Target: $(TARGET)"
	@echo "  Common sources: $(COMMON_SOURCES)"
	@echo "  Platform sources: $(PLATFORM_SOURCES)"
	@echo "  All sources: $(SOURCES)"

# Run with default settings
run: $(TARGET)
	@echo "Running with default settings..."
	./$(TARGET)

# Run cache-aware tests
run-cache: $(TARGET)
	@echo "Running cache hierarchy tests..."
	./$(TARGET) --cache-hierarchy

# macOS specific tests (if on macOS)
ifeq ($(shell uname),Darwin)
run-p-cores: $(TARGET)
	@echo "Running tests on P-cores only..."
	./$(TARGET) --cache-hierarchy --p-cores --threads 12

run-e-cores: $(TARGET)
	@echo "Running tests on E-cores only..."
	./$(TARGET) --cache-hierarchy --e-cores --threads 4
endif

# Format code using clang-format
format:
	@echo "Formatting C++ code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i --style=file; \
		echo "Code formatting complete"; \
	else \
		echo "clang-format not found. Please install clang-format"; \
	fi

# Show help
help: $(TARGET)
	@echo "Showing program help..."
	./$(TARGET) --help

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build (default)
release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

.PHONY: all clean install-deps info run run-cache run-p-cores run-e-cores format help debug release