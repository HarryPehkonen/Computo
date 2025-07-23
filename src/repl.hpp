#pragma once

#include "cli_args.hpp"

namespace computo {

// Run the interactive REPL mode
auto run_repl_mode(const ComputoArgs& args) -> int;

} // namespace computo
