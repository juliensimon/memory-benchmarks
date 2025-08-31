# Memory Bandwidth Test Tool Makefile - Enhanced Optimization Structure
#
# This Makefile builds the memory benchmark tool with comprehensive optimization support
# Supports debug, release, and performance (PGO) build configurations

# Compiler selection with version detection
CXX := $(shell which g++-12 2>/dev/null || which g++-11 2>/dev/null || which g++ 2>/dev/null)
CC := $(shell which gcc-12 2>/dev/null || which gcc-11 2>/dev/null || which gcc 2>/dev/null)

# Build type detection
BUILD_TYPE ?= release
DEBUG ?= 0

# Base flags
BASE_CXXFLAGS = -std=c++17 -pthread -Wall -Wextra -Wpedantic

# Debug build optimizations
DEBUG_CXXFLAGS = $(BASE_CXXFLAGS) \
	-g3 -O0 \
	-DDEBUG \
	-fsanitize=address \
	-fsanitize=undefined \
	-fno-omit-frame-pointer \
	-fstack-protector-strong

# Release build optimizations
RELEASE_CXXFLAGS = $(BASE_CXXFLAGS) \
	-O3 -DNDEBUG \
	-march=native -mtune=native \
	-flto=auto \
	-ffast-math \
	-funroll-loops \
	-finline-functions \
	-fomit-frame-pointer \
	-fno-stack-protector

# Performance build optimizations (for benchmarking with PGO)
# Different PGO flags for GCC vs Clang
ifeq ($(findstring clang,$(CXX)),clang)
    # Clang PGO flags
    PERFORMANCE_CXXFLAGS = $(RELEASE_CXXFLAGS) \
	    -fprofile-instr-generate
    PGO_USE_FLAGS = -fprofile-instr-use=default.profdata
else
    # GCC PGO flags
    PERFORMANCE_CXXFLAGS = $(RELEASE_CXXFLAGS) \
	    -fprofile-generate
    PGO_USE_FLAGS = -fprofile-use -fprofile-correction
endif

# Link-time optimizations (base)
RELEASE_LDFLAGS_BASE = -flto=auto
DEBUG_LDFLAGS = -fsanitize=address -fsanitize=undefined

# Platform-specific release linker flags
ifeq ($(shell uname),Darwin)
    # macOS linker flags
    RELEASE_LDFLAGS = $(RELEASE_LDFLAGS_BASE) -Wl,-dead_strip
else
    # Linux linker flags  
    RELEASE_LDFLAGS = $(RELEASE_LDFLAGS_BASE) -Wl,--gc-sections -s -Wl,--strip-all
endif

# Base linker flags
BASE_LDFLAGS = -pthread

# Build type selection
ifeq ($(BUILD_TYPE),debug)
    CXXFLAGS = $(DEBUG_CXXFLAGS)
    LDFLAGS = $(BASE_LDFLAGS) $(DEBUG_LDFLAGS)
else ifeq ($(BUILD_TYPE),performance)
    CXXFLAGS = $(PERFORMANCE_CXXFLAGS)
    LDFLAGS = $(BASE_LDFLAGS) $(RELEASE_LDFLAGS)
    ifeq ($(findstring clang,$(CXX)),clang)
        LDFLAGS += -fprofile-instr-generate
    else
        LDFLAGS += -fprofile-generate
    endif
else
    CXXFLAGS = $(RELEASE_CXXFLAGS)
    LDFLAGS = $(BASE_LDFLAGS) $(RELEASE_LDFLAGS)
endif

# Platform-specific optimizations for matrix operations
ifeq ($(shell uname),Darwin)
    # macOS - use Accelerate framework for Apple AMX with new CBLAS interface
    LDFLAGS += -framework Accelerate
    CXXFLAGS += -DUSE_ACCELERATE -DACCELERATE_NEW_LAPACK
    # Apple Silicon specific optimizations
    ifeq ($(shell sysctl -n machdep.cpu.brand_string | grep -c "Apple" 2>/dev/null || echo 0),1)
        # Apple Silicon specific (no -mfpu on macOS ARM64)
        ifneq ($(BUILD_TYPE),debug)
            CXXFLAGS += -mcpu=apple-a14
        endif
    endif
else ifeq ($(shell uname),Linux)
    ARCH := $(shell uname -m)
    ifeq ($(ARCH),x86_64)
        # Intel AMX support (requires newer GCC/Clang)
        CXXFLAGS += -mamx-tile -mamx-int8 -mamx-bf16
        # Enhanced x86_64 optimizations for release builds
        ifneq ($(BUILD_TYPE),debug)
            CXXFLAGS += -mavx2 -mfma -mbmi -mbmi2
            # Check for AVX-512 support
            ifneq ($(shell grep -c avx512 /proc/cpuinfo 2>/dev/null || echo 0),0)
                CXXFLAGS += -mavx512f -mavx512cd
            endif
        endif
    else ifeq ($(ARCH),aarch64)
        # ARM SVE support (if available)
        CXXFLAGS += -msve-vector-bits=scalable
        # Enhanced ARM64 optimizations for release builds
        ifneq ($(BUILD_TYPE),debug)
            CXXFLAGS += -mcpu=native
        endif
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
	rm -f *.gcda *.gcno  # Clean up profile data
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
	@echo "  Build Type: $(BUILD_TYPE)"
	@echo "  C++ Flags: $(CXXFLAGS)"
	@echo "  Linker Flags: $(LDFLAGS)"
	@echo "  Target: $(TARGET)"
	@echo "  Common sources: $(COMMON_SOURCES)"
	@echo "  Platform sources: $(PLATFORM_SOURCES)"
	@echo "  All sources: $(SOURCES)"
	@echo ""
	@echo "Available Build Types:"
	@echo "  debug      - Debug build with sanitizers (-g3 -O0)"
	@echo "  release    - Optimized release build (-O3 -flto)"
	@echo "  performance- Performance build with PGO support"
	@echo ""
	@echo "Usage Examples:"
	@echo "  make BUILD_TYPE=debug"
	@echo "  make BUILD_TYPE=release"
	@echo "  make pgo-optimized"

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

# Build type targets
debug:
	$(MAKE) BUILD_TYPE=debug $(TARGET)

# Release build (default)
release:
	$(MAKE) BUILD_TYPE=release $(TARGET)

# Performance build with PGO preparation
performance:
	$(MAKE) BUILD_TYPE=performance $(TARGET)

# Profile-guided optimization targets
pgo-generate:
	@echo "Building with profile generation..."
	$(MAKE) BUILD_TYPE=performance $(TARGET)

pgo-use: CXXFLAGS += $(PGO_USE_FLAGS)
pgo-use:
	@echo "Building with profile data..."
	$(MAKE) $(TARGET)

# Complete PGO optimized build workflow
pgo-optimized: pgo-generate
	@echo "Running profile generation workload..."
ifeq ($(findstring clang,$(CXX)),clang)
	@echo "Using Clang PGO workflow..."
	LLVM_PROFILE_FILE="default.profraw" ./$(TARGET) --cache-hierarchy --size 1024 || true
	LLVM_PROFILE_FILE="default.profraw" ./$(TARGET) --pattern matrix --size 512 || true
	@echo "Converting profile data..."
	llvm-profdata merge -output=default.profdata default.profraw || xcrun llvm-profdata merge -output=default.profdata default.profraw || echo "Warning: Could not convert profile data"
else
	@echo "Using GCC PGO workflow..."
	./$(TARGET) --cache-hierarchy --size 1024 || true
	./$(TARGET) --pattern matrix --size 512 || true
endif
	@echo "Building optimized binary with profile data..."
	$(MAKE) clean
	$(MAKE) pgo-use BUILD_TYPE=performance
	@echo "Profile-guided optimization complete"

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

# Test all build configurations
test-all-builds: clean
	@echo "Testing all build configurations..."
	@echo "Testing debug build..."
	$(MAKE) BUILD_TYPE=debug tests
	$(MAKE) test BUILD_TYPE=debug
	@echo "Testing release build..."
	$(MAKE) clean && $(MAKE) BUILD_TYPE=release tests
	$(MAKE) test BUILD_TYPE=release
	@echo "All build configurations tested successfully"

# Run all tests
test: tests
	@echo "Running test suite..."
	@$(TESTS_DIR)/run_tests.sh

# Run performance regression tests specifically
test-performance: $(TESTS_DIR)/test_performance_regression
	@echo "Running performance regression tests..."
	@$(TESTS_DIR)/test_performance_regression

# Benchmark different build types
benchmark-builds: clean
	@echo "Benchmarking different build configurations..."
	@echo "Building debug version..."
	$(MAKE) BUILD_TYPE=debug $(TARGET)
	mv $(TARGET) $(TARGET)_debug
	@echo "Building release version..."
	$(MAKE) clean && $(MAKE) BUILD_TYPE=release $(TARGET)
	mv $(TARGET) $(TARGET)_release
	@echo "Building PGO version..."
	$(MAKE) clean && $(MAKE) pgo-optimized
	mv $(TARGET) $(TARGET)_pgo
	@echo "Running benchmarks..."
	@echo "Debug build:"
	time ./$(TARGET)_debug --cache-hierarchy --size 512 2>/dev/null || true
	@echo "Release build:"
	time ./$(TARGET)_release --cache-hierarchy --size 512 2>/dev/null || true
	@echo "PGO build:"
	time ./$(TARGET)_pgo --cache-hierarchy --size 512 2>/dev/null || true
	@echo "Benchmark complete. Check timing results above."
	rm -f $(TARGET)_debug $(TARGET)_release $(TARGET)_pgo

# Clean tests
clean-tests:
	@echo "Cleaning test artifacts..."
	rm -f $(TEST_OBJECTS) $(TEST_EXECUTABLES)

# Clean everything including tests and profile data
clean-all: clean clean-tests
	@echo "Cleaning all profile data..."
	rm -rf *.gcda *.gcno *.profdata *.profraw
	@echo "Deep clean complete"

.PHONY: all clean clean-tests clean-all install-deps info run run-cache run-p-cores run-e-cores format help debug release performance pgo-generate pgo-use pgo-optimized tests test