#pragma once

#include <memory>
#include <vector>
#include <cstddef>

class Arena {
public:
    Arena(size_t initial_size = 64 * 1024) // 64KB default
        : m_block_size(initial_size) {
        allocate_new_block();
    }

    ~Arena() {
        // All blocks automatically freed when vector is destroyed
    }

    // Non-copyable, non-movable for simplicity
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;

    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        // Align to T's alignment requirements
        size_t aligned_size = align_size(sizeof(T), alignof(T));
        
        // Check if current block has enough space
        if (m_current_offset + aligned_size > m_current_block->size()) {
            allocate_new_block();
        }

        // Get pointer to allocated memory
        void* ptr = m_current_block->data() + m_current_offset;
        m_current_offset += aligned_size;

        // Construct object in-place
        return new(ptr) T(std::forward<Args>(args)...);
    }

    // Get total memory used
    size_t total_allocated() const {
        size_t total = 0;
        for (const auto& block : m_blocks) {
            total += block->size();
        }
        return total;
    }

    // Get memory actually used (not including waste)
    size_t memory_used() const {
        size_t used = 0;
        for (size_t i = 0; i < m_blocks.size() - 1; ++i) {
            used += m_blocks[i]->size(); // Previous blocks fully used
        }
        used += m_current_offset; // Current block partially used
        return used;
    }

private:
    void allocate_new_block() {
        // Double block size each time to reduce number of allocations
        if (!m_blocks.empty()) {
            m_block_size *= 2;
        }
        
        auto new_block = std::make_unique<std::vector<char>>(m_block_size);
        m_current_block = new_block.get();
        m_current_offset = 0;
        m_blocks.push_back(std::move(new_block));
    }

    static size_t align_size(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    std::vector<std::unique_ptr<std::vector<char>>> m_blocks;
    std::vector<char>* m_current_block = nullptr;
    size_t m_current_offset = 0;
    size_t m_block_size;
}; 