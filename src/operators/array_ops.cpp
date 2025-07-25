#include "operators/shared.hpp"

namespace computo::operators {

auto map_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto processor = [](const nlohmann::json& item, const nlohmann::json& lambda_result,
                        nlohmann::json& final_result) -> bool {
        if (!final_result.is_array()) {
            final_result = nlohmann::json::array();
        }
        final_result.push_back(lambda_result);
        return true; // Continue processing all items
    };

    auto result = process_array_with_lambda(args, ctx, "map", processor);
    // Handle empty arrays
    if (result.is_null()) {
        result = nlohmann::json::array();
    }
    return EvaluationResult(nlohmann::json{{"array", result}});
}

auto filter_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto processor = [](const nlohmann::json& item, const nlohmann::json& lambda_result,
                        nlohmann::json& final_result) -> bool {
        if (!final_result.is_array()) {
            final_result = nlohmann::json::array();
        }
        if (is_truthy(lambda_result)) {
            final_result.push_back(item);
        }
        return true; // Continue processing all items
    };

    auto result = process_array_with_lambda(args, ctx, "filter", processor);
    // Handle empty arrays
    if (result.is_null()) {
        result = nlohmann::json::array();
    }
    return EvaluationResult(nlohmann::json{{"array", result}});
}

auto reduce_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 3) {
        throw InvalidArgumentException(
            "'reduce' requires exactly 3 arguments (array, lambda, initial)",
            ctx.get_path_string());
    }

    auto array_input = evaluate(args[0], ctx);
    auto initial_value = evaluate(args[2], ctx);
    auto array_data = extract_array_data(array_input, "reduce", ctx.get_path_string());

    nlohmann::json accumulator = initial_value;

    for (const auto& item : array_data) {
        std::vector<nlohmann::json> lambda_args = {accumulator, item};
        auto lambda_result = evaluate_lambda(args[1], lambda_args, ctx);

        // Resolve any tail calls from lambda evaluation
        while (lambda_result.is_tail_call) {
            lambda_result = evaluate_internal(lambda_result.tail_call->expression,
                                              lambda_result.tail_call->context);
        }

        accumulator = lambda_result.value;
    }

    return EvaluationResult(accumulator);
}

auto count_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'count' requires exactly 1 argument",
                                       ctx.get_path_string());
    }

    auto array_input = evaluate(args[0], ctx);
    auto array_data = extract_array_data(array_input, "count", ctx.get_path_string());

    return EvaluationResult(static_cast<int>(array_data.size()));
}

auto find_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto processor = [](const nlohmann::json& item, const nlohmann::json& lambda_result,
                        nlohmann::json& final_result) -> bool {
        if (is_truthy(lambda_result)) {
            final_result = item;
            return false; // Found item, stop processing
        }
        return true; // Continue searching
    };

    auto result = process_array_with_lambda(args, ctx, "find", processor);
    // If result is still null (not set by processor), no item was found
    if (result.is_null()) {
        return EvaluationResult(nlohmann::json(nullptr));
    }
    return EvaluationResult(result);
}

auto some_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto processor = [](const nlohmann::json& item, const nlohmann::json& lambda_result,
                        nlohmann::json& final_result) -> bool {
        if (is_truthy(lambda_result)) {
            final_result = true;
            return false; // Found truthy result, stop processing
        }
        return true; // Continue searching
    };

    auto result = process_array_with_lambda(args, ctx, "some", processor);
    // If result is still null (not set by processor), no truthy item was found
    if (result.is_null()) {
        return EvaluationResult(false);
    }
    return EvaluationResult(result);
}

auto every_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    auto processor = [](const nlohmann::json& item, const nlohmann::json& lambda_result,
                        nlohmann::json& final_result) -> bool {
        if (!is_truthy(lambda_result)) {
            final_result = false;
            return false; // Found falsy result, stop processing
        }
        return true; // Continue checking
    };

    auto result = process_array_with_lambda(args, ctx, "every", processor);
    // If result is still null (not set by processor), all items were truthy
    if (result.is_null()) {
        return EvaluationResult(true);
    }
    return EvaluationResult(result);
}

} // namespace computo::operators
