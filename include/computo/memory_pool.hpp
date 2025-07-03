#pragma once

#include <nlohmann/json.hpp>
#include <vector>
#include <memory>
#include <stack>
#include <functional>
#include <unordered_set>
#include <atomic>

namespace computo {
namespace memory {

/**
 * Global memory pool statistics across all threads
 */
class GlobalPoolStats {
private:
    static std::atomic<size_t> total_objects_created_;
    static std::atomic<size_t> total_objects_reused_;
    static std::atomic<size_t> total_pool_hits_;
    static std::atomic<size_t> total_pool_misses_;
    
public:
    static void increment_objects_created() { total_objects_created_++; }
    static void increment_objects_reused() { total_objects_reused_++; }
    static void increment_pool_hits() { total_pool_hits_++; }
    static void increment_pool_misses() { total_pool_misses_++; }
    
    static size_t get_total_objects_created() { return total_objects_created_.load(); }
    static size_t get_total_objects_reused() { return total_objects_reused_.load(); }
    static size_t get_total_pool_hits() { return total_pool_hits_.load(); }
    static size_t get_total_pool_misses() { return total_pool_misses_.load(); }
    
    static double get_pool_hit_rate() {
        size_t hits = total_pool_hits_.load();
        size_t misses = total_pool_misses_.load();
        if (hits + misses == 0) return 0.0;
        return static_cast<double>(hits) / (hits + misses);
    }
    
    static void reset() {
        total_objects_created_ = 0;
        total_objects_reused_ = 0;
        total_pool_hits_ = 0;
        total_pool_misses_ = 0;
    }
};

/**
 * Thread-safe memory pool for JSON objects using index-based management.
 * Eliminates use-after-free risks through stable object storage.
 */
class JsonMemoryPool {
private:
    // Objects stored in stable vector - indices never invalidate
    std::vector<std::unique_ptr<nlohmann::json>> objects_;
    
    // Available object indices (not raw pointers)
    std::stack<size_t> available_indices_;
    
    // Pool configuration
    size_t max_pool_size_;
    size_t total_created_objects_ = 0;
    
    // Pool generation counter - invalidates stale references on clear()
    std::atomic<uint64_t> generation_{1};
    
public:
    explicit JsonMemoryPool(size_t max_size = 1000) : max_pool_size_(max_size) {
        // Pre-allocate some objects with stable indices
        size_t pre_alloc = std::min(max_size, size_t(10));
        objects_.reserve(max_size); // Prevent vector reallocations
        
        for (size_t i = 0; i < pre_alloc; ++i) {
            objects_.emplace_back(std::make_unique<nlohmann::json>());
            available_indices_.push(i);
        }
        total_created_objects_ = pre_alloc;
    }
    
    // Safe handle for pooled objects
    class PooledJsonHandle {
    private:
        nlohmann::json* ptr_;
        size_t index_;
        uint64_t generation_;
        JsonMemoryPool* pool_;
        
    public:
        PooledJsonHandle(nlohmann::json* ptr, size_t index, uint64_t gen, JsonMemoryPool* pool)
            : ptr_(ptr), index_(index), generation_(gen), pool_(pool) {}
            
        ~PooledJsonHandle() {
            if (pool_) {
                pool_->return_to_pool(index_, generation_);
            }
        }
        
        // Non-copyable, movable
        PooledJsonHandle(const PooledJsonHandle&) = delete;
        PooledJsonHandle& operator=(const PooledJsonHandle&) = delete;
        
        PooledJsonHandle(PooledJsonHandle&& other) noexcept
            : ptr_(other.ptr_), index_(other.index_), 
              generation_(other.generation_), pool_(other.pool_) {
            other.pool_ = nullptr; // Transfer ownership
        }
        
        PooledJsonHandle& operator=(PooledJsonHandle&& other) noexcept {
            if (this != &other) {
                if (pool_) {
                    pool_->return_to_pool(index_, generation_);
                }
                ptr_ = other.ptr_;
                index_ = other.index_;
                generation_ = other.generation_;
                pool_ = other.pool_;
                other.pool_ = nullptr;
            }
            return *this;
        }
        
        nlohmann::json& operator*() { return *ptr_; }
        const nlohmann::json& operator*() const { return *ptr_; }
        nlohmann::json* operator->() { return ptr_; }
        const nlohmann::json* operator->() const { return ptr_; }
        nlohmann::json* get() { return ptr_; }
        const nlohmann::json* get() const { return ptr_; }
    };

    /**
     * Acquire a JSON object from the pool.
     * Returns handle with automatic return-to-pool on destruction.
     */
    PooledJsonHandle acquire() {
        size_t index;
        nlohmann::json* ptr;
        
        if (!available_indices_.empty()) {
            // Reuse from pool
            index = available_indices_.top();
            available_indices_.pop();
            ptr = objects_[index].get();
            *ptr = nlohmann::json(nullptr); // Reset to null
            GlobalPoolStats::increment_pool_hits();
            GlobalPoolStats::increment_objects_reused();
        } else {
            // Create new object
            index = objects_.size();
            objects_.emplace_back(std::make_unique<nlohmann::json>());
            ptr = objects_[index].get();
            total_created_objects_++;
            GlobalPoolStats::increment_pool_misses();
            GlobalPoolStats::increment_objects_created();
        }
        
        return PooledJsonHandle(ptr, index, generation_.load(), this);
    }
    
    /**
     * Get pool statistics for monitoring.
     */
    struct Stats {
        size_t total_objects;
        size_t available_objects;
        size_t pool_usage_percent;
    };
    
    Stats get_stats() const {
        Stats stats;
        stats.total_objects = total_created_objects_;
        stats.available_objects = available_indices_.size();
        stats.pool_usage_percent = total_created_objects_ == 0 ? 0 : 
            ((total_created_objects_ - available_indices_.size()) * 100) / total_created_objects_;
        return stats;
    }
    
    /**
     * Clear the entire pool safely. Outstanding handles remain valid
     * but won't be returned to pool due to generation checking.
     */
    void clear() {
        // Increment generation to invalidate outstanding handles for pool return
        generation_++;
        
        // Clear available indices (but keep objects alive for outstanding handles)
        while (!available_indices_.empty()) {
            available_indices_.pop();
        }
        
        // DON'T clear objects_ - they need to remain valid for outstanding handles
        // Only clear the available pool, not the actual objects
        total_created_objects_ = 0;
    }

private:
    /**
     * Internal method to return object to pool by index.
     * Uses generation checking to prevent stale returns.
     */
    void return_to_pool(size_t index, uint64_t handle_generation) {
        // Check if this handle is from the current generation
        if (handle_generation != generation_.load()) {
            // Stale handle from before clear() - ignore safely
            return;
        }
        
        // Check bounds and pool capacity
        if (index < objects_.size() && available_indices_.size() < max_pool_size_) {
            // Reset object and return to pool
            *objects_[index] = nlohmann::json(nullptr);
            available_indices_.push(index);
        }
        // If pool is full or index invalid, object stays allocated but not pooled
    }
};

/**
 * Thread-local memory pool instance.
 * Each thread gets its own pool to avoid synchronization overhead.
 */
inline JsonMemoryPool& get_thread_local_pool() {
    static thread_local JsonMemoryPool pool;
    return pool;
}

// Type alias for the new handle type (backward compatibility helper)
using PooledJsonPtr = JsonMemoryPool::PooledJsonHandle;

} // namespace memory
} // namespace computo 