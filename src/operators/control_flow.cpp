#include "operators/shared.hpp"

namespace computo::operators {

auto if_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 3) {
        throw InvalidArgumentException("'if' requires exactly 3 arguments (condition, then, else)",
                                       ctx.get_path_string());
    }

    // Evaluate condition
    auto condition = evaluate(args[0], ctx.with_path("condition"));

    // Return tail call for TCO optimization
    if (is_truthy(condition)) {
        // Return tail call to then branch
        return {args[1], ctx.with_path("then")};
    }
    // Return tail call to else branch
    return {args[2], ctx.with_path("else")};
}

} // namespace computo::operators
