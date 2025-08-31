# Memory Bandwidth Test Tool

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey.svg)](#multi-platform-support)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](#installation)
[![Architecture](https://img.shields.io/badge/architecture-x86__64%20%7C%20ARM64-orange.svg)](#multi-platform-support)
[![Memory](https://img.shields.io/badge/memory-DDR3%20%7C%20DDR4%20%7C%20DDR5%20%7C%20LPDDR4%20%7C%20LPDDR5-purple.svg)](#hardware-detection-capabilities)
[![Threading](https://img.shields.io/badge/threading-Multithreaded-red.svg)](#threading)
[![Cache](https://img.shields.io/badge/cache-L1%20%7C%20L2%20%7C%20L3-yellow.svg)](#cache-information)

A standalone multithreaded C++ program to measure memory bandwidth between the host CPU and RAM.
This tool provides comprehensive memory performance analysis with various access patterns.

## Table of Contents

- [Features](#features)
- [Test Patterns](#test-patterns)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Output Format](#output-format)
- [Multi-Platform Support](#multi-platform-support)
- [Technical Details](#technical-details)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)
- [License](#license)
- [Contributing](#contributing)

## Features

- **Multiple Test Patterns**: Sequential read/write, random access, copy operations, and STREAM triad tests
- **Multithreaded**: Utilizes all available CPU cores for maximum bandwidth measurement
- **Configurable**: Adjustable buffer size, iteration count, and thread count
- **Cache-Aware Testing**: Tests different working set sizes to measure L1/L2/L3 cache performance
- **Cache Line Optimized**: Optimized for cache line alignment and efficient memory access
- **Comprehensive Metrics**: Measures bandwidth (GB/s), latency (ns), and throughput
- **System Information**: Displays system RAM, CPU name, cores, and cache information
- **Auto-Detection**: Automatically detects cache sizes, CPU characteristics, and memory specifications
- **Multi-Platform Support**: Full support for Linux and macOS with platform-specific optimizations

## Test Patterns

1. **Sequential Read**: Measures read bandwidth by accessing memory sequentially
2. **Sequential Write**: Measures write bandwidth by writing to memory sequentially
3. **Random Read**: Measures random access read performance
4. **Random Write**: Measures random access write performance
5. **Copy**: Measures bandwidth for copying data between buffers (read + write)
6. **Triad**: STREAM triad benchmark (a[i] = b[i] + scalar * c[i])

## Requirements

- **Linux** (Ubuntu, Debian, CentOS, RHEL, etc.) or **macOS** system
- GCC compiler with C++17 support
- pthread library
- Sufficient RAM for testing (default: 1GB, configurable)
- **Optional**: `dmidecode` (Linux) for detailed memory specifications
- **Optional**: `lshw` (Linux) for additional hardware information

## Installation

### Linux

1. **Install build dependencies**:

   ```bash
   # Ubuntu/Debian
   sudo apt-get update
   sudo apt-get install build-essential
   
   # Optional: Install dmidecode and lshw for enhanced hardware detection
   sudo apt-get install dmidecode lshw
   
   # CentOS/RHEL
   sudo yum groupinstall "Development Tools"
   sudo yum install dmidecode lshw
   ```

2. **Compile the program**:

   ```bash
   make
   ```

### macOS

1. **Install build dependencies** (if not already installed):

   ```bash
   # Install Xcode Command Line Tools
   xcode-select --install
   
   # Or install via Homebrew
   brew install gcc
   ```

2. **Compile the program**:

   ```bash
   make
   ```

### Developer Setup (Optional)

```bash
make install-precommit
```

## Usage

### Basic Usage

Run with default settings (1GB buffer, 10 iterations, auto-detect threads):

```bash
./memory_bandwidth
```

### Command Line Options

```bash
./memory_bandwidth [options]
```

**Options:**

- `--size SIZE` - Memory size in GB (default: 1)
- `--iterations N` - Number of iterations (default: 10)
- `--threads N` - Number of threads (default: auto-detect)
- `--pattern PATTERN` - Test pattern: sequential_read, sequential_write, random_read, random_write, copy, triad
  (default: all)
- `--cache-aware` - Run cache-aware tests with different working set sizes
- `--format FORMAT` - Output format: markdown, json, csv (default: markdown)
- `-h, --help` - Show help message

### Examples

**Quick test with small memory footprint**:

```bash
./memory_bandwidth --size 0.5 --iterations 3 --threads 4
```

**Comprehensive test with large buffer**:

```bash
./memory_bandwidth --size 4 --iterations 5 --threads 8
```

**Single-threaded test**:

```bash
./memory_bandwidth --threads 1
```

**Cache-aware test**:

```bash
./memory_bandwidth --cache-aware --iterations 3 --threads 4
```

**Specific pattern test with CSV output**:

```bash
./memory_bandwidth --pattern sequential_read --format csv
```

### Makefile Targets

#### Build Targets
- `make` / `make all` - Compile the program
- `make debug` - Build with debug symbols
- `make release` - Build optimized release version
- `make clean` - Remove compiled binary
- `make clean-all` - Clean everything including tests

#### Run Targets
- `make run` - Run with default settings
- `make run-cache` - Run cache hierarchy tests
- `make run-matrix` - Run matrix multiplication benchmark
- `make run-matrix-small` - Run small matrix benchmark (512x512)
- `make run-matrix-large` - Run large matrix benchmark (2048x2048)

#### macOS-specific Targets (when on macOS)
- `make run-p-cores` - Run tests on P-cores only
- `make run-e-cores` - Run tests on E-cores only
- `make run-matrix-amx` - Run matrix tests with Apple AMX acceleration

#### Test Targets
- `make tests` - Build all test executables
- `make test` - Build and run all tests
- `make test-performance` - Run performance regression tests
- `make clean-tests` - Clean test artifacts

#### Utility Targets
- `make help` - Show program help
- `make info` - Show build information
- `make format` - Format C++ code with clang-format
- `make install-deps` - Install build dependencies

## Output Format

The program displays comprehensive system information and test results in multiple formats:

### System Information
- **CPU Name**: Automatically detected processor model (e.g., "Intel Core i7-10700K", "AMD Ryzen 9 5950X", "AWS Graviton4")
- **RAM**: Total and available system memory
- **CPU Cores**: Physical and logical processor cores
- **Cache Information**: L1/L2/L3 cache sizes and characteristics
- **Memory Specifications**: Type (DDR4/DDR5), speed, data width, channels, theoretical bandwidth

### Test Results
- Test pattern name
- Bandwidth (GB/s and Gb/s)
- Latency (ns)
- Number of threads
- Data size processed (GB)
- Test duration (seconds)
- Efficiency percentage (compared to theoretical bandwidth)

Example output:

```
# System Information

- **CPU:** Intel Core i7-10700K ✓
- **Total RAM:** 16 GB ✓
- **Available RAM:** 12.50 GB ✓
- **Physical CPU Cores:** 8 ✓
- **Logical CPU Threads:** 16 ✓
- **Cache Line Size:** 64 bytes ✓

## Memory Specifications

- **Type:** DDR4 ✓
- **Speed:** 3200 MT/s ✓
- **Data Width:** 64 bits ✓
- **Total Width:** 64 bits ✓
- **Channels:** 2 ✓
- **Theoretical Bandwidth:** 25.6 GB/s (204.8 Gb/s) ✓

## Cache Information

- **L1 Data Cache:** 32 KB per core ✓
- **L1 Instruction Cache:** 32 KB per core ✓
- **L2 Cache:** 256 KB per core ✓
- **L3 Cache:** 16 MB shared ✓
- **Cache Line Size:** 64 bytes ✓

Allocating 1.00 GB of memory...
Memory allocation successful

Test Pattern         Bandwidth    Latency      Threads  Data Size    Time
--------------------------------------------------------------------------------
Sequential Read      25.60 GB/s   2.50 ns      8        10.00 GB     0.39 s
Sequential Write     18.40 GB/s   3.48 ns      8        10.00 GB     0.54 s
Random Read          2.10 GB/s    30.48 ns     8        10.00 GB     4.76 s
Random Write         1.80 GB/s    35.56 ns     8        10.00 GB     5.56 s
Copy                 12.30 GB/s   5.20 ns      8        20.00 GB     1.63 s
Triad                8.20 GB/s    7.80 ns      8        30.00 GB     3.66 s

Test completed successfully!
```

Cache-aware test output:

```
Cache-Aware Sequential Read Test:
------------------------------------------------------------------------------------------
Test Pattern        Working Set Bandwidth         Latency         Threads Data Size       Time
------------------------------------------------------------------------------------------
Sequential Read     1/4 L1      107.85      GB/s  15.26       ns  4       0.00        GB  0.00    s
Sequential Read     1/2 L1      153.83      GB/s  7.85        ns  4       0.00        GB  0.00    s
Sequential Read     L1          61.45       GB/s  11.69       ns  4       0.00        GB  0.00    s
Sequential Read     2x L1       308.49      GB/s  3.80        ns  4       0.00        GB  0.00    s
Sequential Read     1/2 L2      194.66      GB/s  5.44        ns  4       0.00        GB  0.00    s
Sequential Read     L2          350.22      GB/s  3.39        ns  4       0.00        GB  0.00    s
Sequential Read     2x L2       373.22      GB/s  5.05        ns  4       0.00        GB  0.00    s
Sequential Read     1/4 L3      362.82      GB/s  5.15        ns  4       0.00        GB  0.00    s
Sequential Read     1/2 L3      355.71      GB/s  5.59        ns  4       0.00        GB  0.00    s
Sequential Read     L3          392.69      GB/s  4.77        ns  4       0.01        GB  0.00    s
Sequential Read     2x L3       344.47      GB/s  5.55        ns  4       0.01        GB  0.00    s
Sequential Read     4x L3       381.76      GB/s  5.23        ns  4       0.02        GB  0.00    s
Sequential Read     64MB        386.14      GB/s  5.24        ns  4       0.04        GB  0.00    s
Sequential Read     256MB       372.95      GB/s  5.47        ns  4       0.17        GB  0.00    s
Sequential Read     1GB         366.03      GB/s  5.58        ns  4       0.67        GB  0.02    s
```

## Multi-Platform Support

This tool provides comprehensive support for both Linux and macOS systems with platform-specific optimizations and hardware detection.

### Platform-Specific Features

#### Linux Support
- **CPU Detection**: Reads from `/proc/cpuinfo` for processor information
- **Memory Detection**: Uses `dmidecode` for detailed memory specifications (DDR4/DDR5 type, speed, channels)
- **Cache Detection**: Multiple detection methods including `getconf`, sysfs, and `lscpu`
- **Hardware Info**: Optional `lshw` integration for additional hardware details
- **ARM Support**: Full support for ARM processors including AWS Graviton series

#### macOS Support
- **CPU Detection**: Uses `sysctl machdep.cpu.brand_string` for processor information
- **Memory Detection**: Apple Silicon memory specifications (LPDDR4/LPDDR5)
- **Cache Detection**: Native `sysctl` APIs for cache line size and characteristics
- **Apple Silicon**: Optimized detection for M1/M2/M3 series processors

### Hardware Detection Capabilities

#### CPU Information
- **Processor Name**: Automatically detected from system APIs
- **Architecture**: x86_64, ARM64 (Apple Silicon, AWS Graviton, etc.)
- **Core Count**: Physical and logical processor cores
- **Threading**: Hyper-threading/SMT detection

#### Memory Specifications
- **Type**: DDR3, DDR4, DDR5, LPDDR4, LPDDR5
- **Speed**: Memory transfer rate in MT/s
- **Data Width**: Memory bus width in bits
- **Channels**: Number of memory channels
- **Theoretical Bandwidth**: Calculated maximum bandwidth

#### Cache Information
- **L1 Cache**: Data and instruction cache sizes per core
- **L2 Cache**: Unified cache size per core
- **L3 Cache**: Shared cache size
- **Cache Line Size**: Optimized memory access alignment

## Technical Details

### Memory Allocation

- Buffers are aligned to cache line boundaries (64 bytes) for optimal performance
- Multiple buffers are allocated for tests requiring multiple memory regions
- Memory is allocated using `new[]` with proper alignment

### Threading

- Each thread is pinned to a specific CPU core using `pthread_setaffinity_np`
- Work is distributed evenly across threads
- Thread synchronization is minimal to avoid overhead

### Performance Measurement

- Uses `std::chrono::high_resolution_clock` for precise timing
- Bandwidth calculation: `(bytes_processed * 8) / (time_seconds * 1e9)` GB/s
- Latency calculation: `(time_nanoseconds) / (number_of_operations)`

### Optimizations

- Compiler optimizations: `-O3 -march=native -mtune=native`
- Cache line aligned memory access
- Volatile variables to prevent compiler optimization of read operations
- Efficient memory access patterns

## API Reference

### Core Classes

#### `PlatformInterface`
Abstract base class for platform-specific implementations.

**Key Methods:**
- `detect_cpu_info()` - Detects CPU model and characteristics
- `detect_memory_info()` - Detects system memory specifications
- `detect_cache_info()` - Detects cache hierarchy information
- `get_core_count()` - Returns physical and logical core counts

#### `StandardTests`
Main test execution engine for memory benchmarks.

**Key Methods:**
- `run_tests()` - Executes all configured test patterns
- `run_cache_aware_tests()` - Runs cache-hierarchy-aware benchmarks
- `set_thread_count()` - Configures threading for tests

#### `SafeFileUtils`
Secure file reading utilities for system information.

**Key Methods:**
- `read_single_line()` - Safely reads one line from system files
- `read_all_lines()` - Safely reads multiple lines with bounds checking
- `is_safe_path()` - Validates file paths against security threats

#### `AlignedBuffer`
Cache-line-aligned memory buffer management.

**Key Methods:**
- `allocate()` - Allocates aligned memory buffers
- `get_ptr()` - Returns pointer to aligned memory
- `get_size()` - Returns buffer size in bytes

#### `OutputFormatter`
Formats test results in multiple output formats.

**Supported Formats:**
- Markdown (default)
- JSON
- CSV

### Platform-Specific Classes

#### Linux: `IntelPlatform`, `ARM64Platform`
#### macOS: `MacOSPlatform`

Each platform class implements hardware detection and optimization for specific architectures.

### Test Patterns

Available test patterns (defined in `TestPatterns` enum):
- `SEQUENTIAL_READ` - Sequential memory read operations
- `SEQUENTIAL_WRITE` - Sequential memory write operations  
- `RANDOM_READ` - Random access read operations
- `RANDOM_WRITE` - Random access write operations
- `COPY` - Memory copy operations (read + write)
- `TRIAD` - STREAM triad benchmark pattern

### Error Handling

The codebase uses consistent error handling through:
- Return value checking (boolean success/failure)
- Exception handling for memory allocation
- Safe file operations with path validation
- Bounds checking for all memory operations

## Troubleshooting

### Out of Memory

If you encounter memory allocation failures:

- Reduce the buffer size using `--size`
- Close other applications to free RAM
- Use fewer threads with `--threads`

### Poor Performance

- Ensure the program has sufficient CPU priority
- Run with `sudo` for maximum performance (if needed)
- Check if other processes are consuming CPU/memory

### Compilation Issues

- Ensure GCC version supports C++17
- **Linux**: Install build-essential package: `sudo apt-get install build-essential`
- **macOS**: Install GCC via Homebrew: `brew install gcc` (if needed)

### Platform-Specific Issues

#### Linux
- **Permission Denied**: Some hardware detection requires root access. Run with `sudo` if needed
- **Missing dmidecode**: Install with `sudo apt-get install dmidecode` for enhanced memory detection
- **ARM Processor Detection**: AWS Graviton and other ARM processors are fully supported

#### macOS
- **Xcode Command Line Tools**: Ensure they are installed: `xcode-select --install`
- **Apple Silicon**: M1/M2/M3 processors are fully supported with optimized detection
- **Memory Detection**: Apple Silicon uses unified memory architecture, specifications are estimated based on processor model

## License

This program is provided as-is for educational and testing purposes.

## Contributing

Feel free to submit improvements, bug fixes, or additional test patterns.
