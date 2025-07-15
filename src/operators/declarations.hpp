#pragma once
#include <computo.hpp>

namespace computo::operators {
// Arithmetic
auto addition(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto subtraction(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto multiplication(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto division(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto modulo(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// Comparison
auto greater_than(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto less_than(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto greater_equal(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto less_equal(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto equal(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto not_equal(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// Logical
auto logical_and(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto logical_or(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto logical_not(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// Data access / construction / control (stubs for now)
auto var_access(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto let_binding(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto obj_construct(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto if_operator(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// Array operations
auto map_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto filter_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto reduce_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto count_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto find_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto some_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto every_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto zip_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// Functional
auto car_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto cdr_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto cons_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto append_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// Functional programming
auto lambda_operator(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto call_operator(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// Utilities
auto str_concat(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto merge_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto approx_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// Object operations
auto keys_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto values_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto objFromPairs_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto pick_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto omit_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// String operations
auto split_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto join_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto trim_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto upper_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto lower_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;

// Array operations
auto sort_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto reverse_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
auto unique_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json;
}