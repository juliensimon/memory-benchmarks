#ifndef ALIGNED_BUFFER_H
#define ALIGNED_BUFFER_H

#include <memory>
#include <cstddef>
#include <cstdint>

/**
 * @brief RAII wrapper for cache-line aligned memory buffers
 * 
 * Provides automatic memory management for aligned buffers with guaranteed
 * cache line alignment for optimal memory performance.
 */
class AlignedBuffer {
public:
    /**
     * @brief Construct aligned buffer with specified size and alignment
     * @param size Buffer size in bytes
     * @param alignment Alignment requirement (must be power of 2)
     * @throws std::bad_alloc if allocation fails
     * @throws std::invalid_argument if alignment is not power of 2
     */
    AlignedBuffer(size_t size, size_t alignment);
    
    // No copy constructor/assignment (unique ownership)
    AlignedBuffer(const AlignedBuffer&) = delete;
    AlignedBuffer& operator=(const AlignedBuffer&) = delete;
    
    // Move constructor/assignment allowed
    AlignedBuffer(AlignedBuffer&& other) noexcept;
    AlignedBuffer& operator=(AlignedBuffer&& other) noexcept;
    
    // Automatic cleanup
    ~AlignedBuffer() = default;
    
    // Access methods
    uint8_t* data() noexcept { return aligned_ptr_; }
    const uint8_t* data() const noexcept { return aligned_ptr_; }
    size_t size() const noexcept { return size_; }
    size_t alignment() const noexcept { return alignment_; }
    
    // Array access operators
    uint8_t& operator[](size_t index) noexcept { return aligned_ptr_[index]; }
    const uint8_t& operator[](size_t index) const noexcept { return aligned_ptr_[index]; }
    
    // Initialize buffer with pattern
    void initialize_pattern();
    
    // Verify alignment
    bool is_aligned() const noexcept;

private:
    std::unique_ptr<uint8_t[]> raw_buffer_;
    uint8_t* aligned_ptr_;
    size_t size_;
    size_t alignment_;
    
    void calculate_aligned_pointer();
    static bool is_power_of_two(size_t n) noexcept;
};

#endif // ALIGNED_BUFFER_H