#include <computo/computo.hpp>
#include <computo/debugger.hpp>
#include <memory>

namespace computo {

// Global debugger instance
static std::unique_ptr<Debugger> global_debugger = nullptr;

void set_debugger(std::unique_ptr<Debugger> debugger) {
    global_debugger = std::move(debugger);
    if (global_debugger) {
        global_debugger->start_execution();
    }
}

Debugger* get_debugger() {
    return global_debugger.get();
}

} // namespace computo 