#include "declarations.hpp"

namespace computo::operators {

#define STUB_IMPL(name) \
    nlohmann::json name(const nlohmann::json&, ExecutionContext&) { \
        throw InvalidOperatorException(#name " not implemented"); \
    }

// Data access/control
// get_ptr implemented in data.cpp
// STUB_IMPL(obj_construct)

// Array operators implemented in array.cpp

// Functional operators implemented in functional.cpp

// Utility operators implemented in utilities.cpp

#undef STUB_IMPL

} 