#include <computo/memory_pool.hpp>
#include <atomic>

namespace computo {
namespace memory {

// Define static atomic members for GlobalPoolStats
std::atomic<size_t> GlobalPoolStats::total_objects_created_{0};
std::atomic<size_t> GlobalPoolStats::total_objects_reused_{0};
std::atomic<size_t> GlobalPoolStats::total_pool_hits_{0};
std::atomic<size_t> GlobalPoolStats::total_pool_misses_{0};

} // namespace memory
} // namespace computo 