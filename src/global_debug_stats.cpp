#include <computo/debugger.hpp>
#include <atomic>
#include <chrono>

namespace computo {

// Define static atomic members for GlobalDebugStats
std::atomic<size_t> GlobalDebugStats::total_operations_{0};
std::atomic<size_t> GlobalDebugStats::total_errors_{0};
std::atomic<size_t> GlobalDebugStats::total_breakpoints_hit_{0};
std::atomic<std::chrono::milliseconds::rep> GlobalDebugStats::total_execution_time_ms_{0};

} // namespace computo 