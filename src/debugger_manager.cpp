#include <computo/computo.hpp>
#include <computo/debugger.hpp>
#include <memory>
#include <stack>

namespace computo {

// Thread-local debugger instance - each thread has its own debugger
thread_local std::unique_ptr<Debugger> global_debugger = nullptr;

// Thread-local stack for RAII debugger management
thread_local std::stack<std::unique_ptr<Debugger>> debugger_stack;

void set_debugger(std::unique_ptr<Debugger> debugger) {
    global_debugger = std::move(debugger);
    if (global_debugger) {
        global_debugger->start_execution();
    }
}

Debugger* get_debugger() {
    return global_debugger.get();
}

// Push current debugger state onto the stack
void push_debugger_state() {
    debugger_stack.push(std::move(global_debugger));
    global_debugger = nullptr;
}

// Pop previous debugger state from the stack
void pop_debugger_state() {
    if (!debugger_stack.empty()) {
        global_debugger = std::move(debugger_stack.top());
        debugger_stack.pop();
        
        // Restart execution if we have a debugger
        if (global_debugger) {
            global_debugger->start_execution();
        }
    }
}

// DebuggerScope implementation
DebuggerScope::DebuggerScope(std::unique_ptr<Debugger> debugger) 
    : state_pushed_(false) {
    push_debugger_state();
    state_pushed_ = true;
    set_debugger(std::move(debugger));
}

DebuggerScope::~DebuggerScope() {
    if (state_pushed_) {
        pop_debugger_state();
    }
}

Debugger* DebuggerScope::get() const {
    return get_debugger();
}

bool DebuggerScope::is_set() const {
    return get_debugger() != nullptr;
}

} // namespace computo 