#pragma once

#include <nlohmann/json.hpp>
#include <vector>
#include <memory>
#include <stack>
#include <functional>
#include <unordered_set>

namespace computo {
namespace memory {

/**
 * Simple memory pool for JSON objects to reduce allocations
 * during intensive array operations.
 */
class JsonMemoryPool {
private:
    std::stack<nlohmann::json*> available_objects;
    std::vector<std::unique_ptr<nlohmann::json>> all_objects;
    std::unordered_set<nlohmann::json*> managed_objects; // Track which objects we manage
    size_t max_pool_size;
    size_t total_created_objects = 0; // Track all objects, including those created on-demand
    
public:
    explicit JsonMemoryPool(size_t max_size = 1000) : max_pool_size(max_size) {
        // Pre-allocate some objects (but less for large pools)
        size_t pre_alloc = std::min(max_size, size_t(10)); // Cap at 10 for simplicity
        for (size_t i = 0; i < pre_alloc; ++i) {
            auto obj = std::make_unique<nlohmann::json>();
            auto* raw_ptr = obj.get();
            available_objects.push(raw_ptr);
            managed_objects.insert(raw_ptr);
            all_objects.push_back(std::move(obj));
        }
        total_created_objects = pre_alloc;
    }
    
    // Custom deleter type for pool return
    using PoolDeleter = std::function<void(nlohmann::json*)>;
    using PooledJsonPtr = std::unique_ptr<nlohmann::json, PoolDeleter>;

    /**
     * Acquire a JSON object from the pool.
     * Returns a new object if pool is empty.
     */
    PooledJsonPtr acquire() {
        nlohmann::json* raw_ptr = nullptr;
        
        if (!available_objects.empty()) {
            // Reuse from pool
            raw_ptr = available_objects.top();
            available_objects.pop();
            *raw_ptr = nlohmann::json(nullptr); // Reset to null
        } else {
            // Pool is empty, create new object and track it
            auto new_obj = std::make_unique<nlohmann::json>();
            raw_ptr = new_obj.get();
            managed_objects.insert(raw_ptr);
            all_objects.push_back(std::move(new_obj));
            total_created_objects++;
        }
        
        // Create a custom deleter that returns to pool
        PoolDeleter deleter = [this](nlohmann::json* ptr) {
            this->return_to_pool(ptr);
        };
        
        return PooledJsonPtr(raw_ptr, deleter);
    }
    
    /**
     * Internal method to return object to pool.
     */
    void return_to_pool(nlohmann::json* obj) {
        // Only accept objects we manage
        if (managed_objects.find(obj) != managed_objects.end()) {
            if (available_objects.size() < max_pool_size) {
                *obj = nlohmann::json(nullptr); // Reset to null for reuse
                available_objects.push(obj);
            }
            // If pool is full, object stays managed but not immediately available
        }
        // Non-managed objects are ignored (shouldn't happen with our design)
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
        stats.total_objects = total_created_objects;
        stats.available_objects = available_objects.size();
        stats.pool_usage_percent = total_created_objects == 0 ? 0 : 
            ((total_created_objects - available_objects.size()) * 100) / total_created_objects;
        return stats;
    }
    
    /**
     * Clear the entire pool (useful for testing or cleanup).
     */
    void clear() {
        while (!available_objects.empty()) {
            available_objects.pop();
        }
        all_objects.clear();
        managed_objects.clear();
        total_created_objects = 0;
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

} // namespace memory
} // namespace computo 