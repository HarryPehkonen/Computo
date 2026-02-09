#include "operators/shared.hpp"

namespace computo::operators {

auto car_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'car' requires exactly 1 argument", ctx.get_path_string());
    }

    auto array_input = evaluate(args[0], ctx);
    auto array_data = extract_array_data(array_input, "car", ctx.get_path_string(), ctx.array_key);

    if (array_data.empty()) {
        throw InvalidArgumentException("'car' cannot be applied to empty array",
                                       ctx.get_path_string());
    }

    return EvaluationResult(array_data[0]);
}

auto cdr_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'cdr' requires exactly 1 argument", ctx.get_path_string());
    }

    auto array_input = evaluate(args[0], ctx);
    auto array_data = extract_array_data(array_input, "cdr", ctx.get_path_string(), ctx.array_key);

    if (array_data.empty()) {
        throw InvalidArgumentException("'cdr' cannot be applied to empty array",
                                       ctx.get_path_string());
    }

    jsom::JsonDocument result = jsom::JsonDocument::make_array();

    // Add all elements except the first
    for (size_t i = 1; i < array_data.size(); ++i) {
        result.push_back(array_data[i]);
    }

    return EvaluationResult(jsom::JsonDocument{{ctx.array_key, result}});
}

auto cons_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 2) {
        throw InvalidArgumentException("'cons' requires exactly 2 arguments (item, array)",
                                       ctx.get_path_string());
    }

    auto item = evaluate(args[0], ctx);
    auto array_input = evaluate(args[1], ctx);
    auto array_data = extract_array_data(array_input, "cons", ctx.get_path_string(), ctx.array_key);

    jsom::JsonDocument result = jsom::JsonDocument::make_array();

    // Add the item first
    result.push_back(item);

    // Add all elements from the array
    for (const auto& element : array_data) {
        result.push_back(element);
    }

    return EvaluationResult(jsom::JsonDocument{{ctx.array_key, result}});
}

auto append_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'append' requires at least 1 argument",
                                       ctx.get_path_string());
    }

    jsom::JsonDocument result = jsom::JsonDocument::make_array();

    for (const auto& arg_expr : args) {
        auto array_input = evaluate(arg_expr, ctx);
        auto array_data
            = extract_array_data(array_input, "append", ctx.get_path_string(), ctx.array_key);

        // Add all elements from this array to the result
        for (const auto& element : array_data) {
            result.push_back(element);
        }
    }

    return EvaluationResult(jsom::JsonDocument{{ctx.array_key, result}});
}

} // namespace computo::operators
