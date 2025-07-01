#pragma once

#include <nlohmann/json.hpp>
#include <vector>
#include <memory>
#include <stack>

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
    size_t max_pool_size;
    
public:
    explicit JsonMemoryPool(size_t max_size = 1000) : max_pool_size(max_size) {
        // Pre-allocate some objects
        for (size_t i = 0; i < std::min(max_size / 10, size_t(100)); ++i) {
            auto obj = std::make_unique<nlohmann::json>();
            available_objects.push(obj.get());
            all_objects.push_back(std::move(obj));
        }
    }
    
    /**
     * Acquire a JSON object from the pool.
     * Returns a new object if pool is empty.
     */
    std::unique_ptr<nlohmann::json> acquire() {
        if (!available_objects.empty()) {
            auto* raw_ptr = available_objects.top();
            available_objects.pop();
            raw_ptr->clear(); // Reset the object
            
            // Create a custom deleter that returns to pool
            auto deleter = [this](nlohmann::json* ptr) {
                this->return_to_pool(ptr);
            };
            return std::unique_ptr<nlohmann::json, decltype(deleter)>(raw_ptr, deleter);
        }
        
        // Pool is empty, create new object with regular deleter
        return std::make_unique<nlohmann::json>();
    }
    
    /**
     * Internal method to return object to pool.
     */
    void return_to_pool(nlohmann::json* obj) {
        if (available_objects.size() < max_pool_size) {
            obj->clear(); // Clear content for reuse
            available_objects.push(obj);
        }
        // If pool is full, object will be destroyed when all_objects is destroyed
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
        stats.total_objects = all_objects.size();
        stats.available_objects = available_objects.size();
        stats.pool_usage_percent = all_objects.empty() ? 0 : 
            ((all_objects.size() - available_objects.size()) * 100) / all_objects.size();
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