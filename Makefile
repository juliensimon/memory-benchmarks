# Memory Bandwidth Test Tool Makefile - Refactored Structure
#
# This Makefile builds the memory benchmark tool with platform-specific implementations

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -mtune=native -pthread -Wall -Wextra
LDFLAGS = -pthread

# Platform-specific optimizations for matrix operations
ifeq ($(shell uname),Darwin)
    # macOS - use Accelerate framework for Apple AMX with new CBLAS interface
    LDFLAGS += -framework Accelerate
    CXXFLAGS += -DUSE_ACCELERATE -DACCELERATE_NEW_LAPACK
else ifeq ($(shell uname),Linux)
    ARCH := $(shell uname -m)
    ifeq ($(ARCH),x86_64)
        # Intel AMX support (requires newer GCC/Clang)
        CXXFLAGS += -mamx-tile -mamx-int8 -mamx-bf16
    else ifeq ($(ARCH),aarch64)
        # ARM SVE support (if available)
        CXXFLAGS += -msve-vector-bits=scalable
    endif
endif

# Target executable
TARGET = memory_bandwidth

# Directory structure
COMMON_DIR = common
PLATFORM_DIR = platforms
TESTS_DIR = tests

# Common source files
COMMON_SOURCES = $(COMMON_DIR)/test_patterns.cpp \
                $(COMMON_DIR)/standard_tests.cpp \
                $(COMMON_DIR)/matrix_multiply_utils.cpp \
                $(COMMON_DIR)/aligned_buffer.cpp \
                $(COMMON_DIR)/output_formatter.cpp \
                $(COMMON_DIR)/output_formatter_utils.cpp \
                $(COMMON_DIR)/working_sets.cpp \
                $(COMMON_DIR)/platform_factory.cpp \
                $(COMMON_DIR)/argument_parser.cpp \
                $(COMMON_DIR)/system_info_display.cpp \
                $(COMMON_DIR)/memory_utils.cpp \
                $(COMMON_DIR)/safe_file_utils.cpp

# Main source
MAIN_SOURCE = main.cpp

# Platform-specific sources (auto-detected)
PLATFORM_SOURCES = 
ifeq ($(shell uname),Darwin)
    # macOS
    PLATFORM_SOURCES += $(PLATFORM_DIR)/macos/macos_platform.cpp
    PLATFORM_SOURCES += $(PLATFORM_DIR)/macos/macos_matrix_multiplier.cpp
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

# Matrix multiplication tests
run-matrix: $(TARGET)
	@echo "Running matrix multiplication benchmark..."
	./$(TARGET) --pattern matrix --size 1024

run-matrix-small: $(TARGET)
	@echo "Running small matrix multiplication benchmark..."
	./$(TARGET) --pattern matrix --size 512

run-matrix-large: $(TARGET)
	@echo "Running large matrix multiplication benchmark..."
	./$(TARGET) --pattern matrix --size 2048

# macOS specific tests (if on macOS)
ifeq ($(shell uname),Darwin)
run-p-cores: $(TARGET)
	@echo "Running tests on P-cores only..."
	./$(TARGET) --cache-hierarchy --p-cores --threads 12

run-e-cores: $(TARGET)
	@echo "Running tests on E-cores only..."
	./$(TARGET) --cache-hierarchy --e-cores --threads 4

run-matrix-amx: $(TARGET)
	@echo "Running matrix multiplication with Apple AMX acceleration..."
	./$(TARGET) --pattern matrix --size 1024 --p-cores
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

# Test targets
TEST_SOURCES = $(TESTS_DIR)/test_aligned_buffer.cpp \
              $(TESTS_DIR)/test_argument_parser.cpp \
              $(TESTS_DIR)/test_platform_factory.cpp \
              $(TESTS_DIR)/test_safe_file_utils.cpp \
              $(TESTS_DIR)/test_output_formatter.cpp \
              $(TESTS_DIR)/test_performance_regression.cpp \
              $(TESTS_DIR)/test_matrix_multiply_utils.cpp \
              $(TESTS_DIR)/test_memory_utils.cpp \
              $(TESTS_DIR)/test_output_formatter_utils.cpp \
              $(TESTS_DIR)/test_system_info_display_simple.cpp \
              $(TESTS_DIR)/test_test_patterns.cpp \
              $(TESTS_DIR)/test_working_sets.cpp

TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)
TEST_EXECUTABLES = $(TESTS_DIR)/test_aligned_buffer \
                   $(TESTS_DIR)/test_argument_parser \
                   $(TESTS_DIR)/test_platform_factory \
                   $(TESTS_DIR)/test_safe_file_utils \
                   $(TESTS_DIR)/test_output_formatter \
                   $(TESTS_DIR)/test_performance_regression \
                   $(TESTS_DIR)/test_matrix_multiply_utils \
                   $(TESTS_DIR)/test_memory_utils \
                   $(TESTS_DIR)/test_output_formatter_utils \
                   $(TESTS_DIR)/test_system_info_display_simple \
                   $(TESTS_DIR)/test_test_patterns \
                   $(TESTS_DIR)/test_working_sets

# Build all tests
tests: $(TEST_EXECUTABLES)

# Individual test executables
$(TESTS_DIR)/test_aligned_buffer: $(TESTS_DIR)/test_aligned_buffer.o $(COMMON_DIR)/aligned_buffer.o
	@echo "Linking test_aligned_buffer..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_argument_parser: $(TESTS_DIR)/test_argument_parser.o $(COMMON_DIR)/argument_parser.o $(COMMON_DIR)/platform_factory.o $(PLATFORM_SOURCES:.cpp=.o) $(COMMON_DIR)/matrix_multiply_utils.o $(COMMON_DIR)/safe_file_utils.o
	@echo "Linking test_argument_parser..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_platform_factory: $(TESTS_DIR)/test_platform_factory.o $(COMMON_DIR)/platform_factory.o $(PLATFORM_SOURCES:.cpp=.o) $(COMMON_DIR)/matrix_multiply_utils.o $(COMMON_DIR)/safe_file_utils.o
	@echo "Linking test_platform_factory..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_safe_file_utils: $(TESTS_DIR)/test_safe_file_utils.o $(COMMON_DIR)/safe_file_utils.o
	@echo "Linking test_safe_file_utils..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_output_formatter: $(TESTS_DIR)/test_output_formatter.o $(COMMON_DIR)/output_formatter.o $(COMMON_DIR)/output_formatter_utils.o
	@echo "Linking test_output_formatter..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_performance_regression: $(TESTS_DIR)/test_performance_regression.o $(COMMON_DIR)/standard_tests.o $(COMMON_DIR)/aligned_buffer.o $(COMMON_DIR)/test_patterns.o $(COMMON_DIR)/memory_utils.o $(COMMON_DIR)/platform_factory.o $(COMMON_DIR)/matrix_multiply_utils.o $(PLATFORM_SOURCES:.cpp=.o) $(COMMON_DIR)/safe_file_utils.o
	@echo "Linking test_performance_regression..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_matrix_multiply_utils: $(TESTS_DIR)/test_matrix_multiply_utils.o $(COMMON_DIR)/matrix_multiply_utils.o
	@echo "Linking test_matrix_multiply_utils..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_memory_utils: $(TESTS_DIR)/test_memory_utils.o $(COMMON_DIR)/memory_utils.o
	@echo "Linking test_memory_utils..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_output_formatter_utils: $(TESTS_DIR)/test_output_formatter_utils.o $(COMMON_DIR)/output_formatter_utils.o
	@echo "Linking test_output_formatter_utils..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_system_info_display_simple: $(TESTS_DIR)/test_system_info_display_simple.o
	@echo "Linking test_system_info_display_simple..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_test_patterns: $(TESTS_DIR)/test_test_patterns.o $(COMMON_DIR)/test_patterns.o
	@echo "Linking test_test_patterns..."
	$(CXX) $^ $(LDFLAGS) -o $@

$(TESTS_DIR)/test_working_sets: $(TESTS_DIR)/test_working_sets.o $(COMMON_DIR)/working_sets.o
	@echo "Linking test_working_sets..."
	$(CXX) $^ $(LDFLAGS) -o $@

# Run all tests
test: tests
	@echo "Running test suite..."
	@$(TESTS_DIR)/run_tests.sh

# Run performance regression tests specifically
test-performance: $(TESTS_DIR)/test_performance_regression
	@echo "Running performance regression tests..."
	@$(TESTS_DIR)/test_performance_regression

# Clean tests
clean-tests:
	@echo "Cleaning test artifacts..."
	rm -f $(TEST_OBJECTS) $(TEST_EXECUTABLES)

# Clean everything including tests
clean-all: clean clean-tests

.PHONY: all clean clean-tests clean-all install-deps info run run-cache run-p-cores run-e-cores format help debug release tests test