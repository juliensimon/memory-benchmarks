#include "aligned_buffer.h"
#include "errors.h"
#include <new>
#include <cstdint>

AlignedBuffer::AlignedBuffer(size_t size, size_t alignment) 
    : aligned_ptr_(nullptr), size_(size), alignment_(alignment) {
    
    if (size == 0) {
        throw MemoryError("Buffer size cannot be zero");
    }
    
    if (!is_power_of_two(alignment)) {
        throw MemoryError("Alignment must be a power of 2");
    }
    
    // Allocate extra space to ensure we can achieve alignment
    // Check for overflow before allocation
    if (size > SIZE_MAX - alignment) {
        throw MemoryError("Buffer size would cause overflow");
    }
    size_t total_size = size + alignment;
    raw_buffer_ = std::make_unique<uint8_t[]>(total_size);
    
    calculate_aligned_pointer();
    initialize_pattern();
}

AlignedBuffer::AlignedBuffer(AlignedBuffer&& other) noexcept
    : raw_buffer_(std::move(other.raw_buffer_))
    , aligned_ptr_(other.aligned_ptr_)
    , size_(other.size_)
    , alignment_(other.alignment_) {
    
    other.aligned_ptr_ = nullptr;
    other.size_ = 0;
    other.alignment_ = 0;
}

AlignedBuffer& AlignedBuffer::operator=(AlignedBuffer&& other) noexcept {
    if (this != &other) {
        raw_buffer_ = std::move(other.raw_buffer_);
        aligned_ptr_ = other.aligned_ptr_;
        size_ = other.size_;
        alignment_ = other.alignment_;
        
        other.aligned_ptr_ = nullptr;
        other.size_ = 0;
        other.alignment_ = 0;
    }
    return *this;
}

void AlignedBuffer::calculate_aligned_pointer() {
    /**
     * Cache Line Aligned Buffer Calculation
     * 
     * Problem: std::unique_ptr doesn't guarantee cache line alignment
     * Solution: Allocate extra space and manually align the buffer
     * 
     * Step 1: Get raw pointer address as integer
     * Step 2: Round UP to next alignment boundary using bit masking
     *   - Formula: (addr + alignment - 1) & ~(alignment - 1)
     *   - The mask ~(alignment - 1) clears the lower bits to align
     *   - Example: addr=0x1009, alignment=64 (0x40)
     *     → (0x1009 + 0x3F) & ~0x3F → 0x1048 & 0xFFC0 → 0x1040
     * Step 3: Convert back to pointer for aligned memory access
     */
    uintptr_t addr = reinterpret_cast<uintptr_t>(raw_buffer_.get());
    uintptr_t aligned_addr = (addr + alignment_ - 1) & ~(alignment_ - 1);
    aligned_ptr_ = reinterpret_cast<uint8_t*>(aligned_addr);
}

void AlignedBuffer::initialize_pattern() {
    // Initialize buffer with a simple pattern for testing
    for (size_t i = 0; i < size_; ++i) {
        aligned_ptr_[i] = static_cast<uint8_t>(i & 0xFF);
    }
}

bool AlignedBuffer::is_aligned() const noexcept {
    uintptr_t addr = reinterpret_cast<uintptr_t>(aligned_ptr_);
    return (addr & (alignment_ - 1)) == 0;
}

bool AlignedBuffer::is_power_of_two(size_t n) noexcept {
    return n > 0 && (n & (n - 1)) == 0;
}