#pragma once
#include <computo.hpp>

namespace computo::operators {
// Arithmetic
nlohmann::json addition(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json subtraction(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json multiplication(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json division(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json modulo(const nlohmann::json& args, ExecutionContext& ctx);

// Comparison
nlohmann::json greater_than(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json less_than(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json greater_equal(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json less_equal(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json equal(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json not_equal(const nlohmann::json& args, ExecutionContext& ctx);

// Logical
nlohmann::json logical_and(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json logical_or(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json logical_not(const nlohmann::json& args, ExecutionContext& ctx);

// Data access / construction / control (stubs for now)
nlohmann::json var_access(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json let_binding(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json obj_construct(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json if_operator(const nlohmann::json& args, ExecutionContext& ctx);

// Array operations
nlohmann::json map_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json filter_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json reduce_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json count_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json find_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json some_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json every_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json zip_op(const nlohmann::json& args, ExecutionContext& ctx);

// Functional
nlohmann::json car_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json cdr_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json cons_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json append_op(const nlohmann::json& args, ExecutionContext& ctx);

// Functional programming
nlohmann::json lambda_operator(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json call_operator(const nlohmann::json& args, ExecutionContext& ctx);

// Utilities
nlohmann::json str_concat(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json merge_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json approx_op(const nlohmann::json& args, ExecutionContext& ctx);

// Object operations
nlohmann::json keys_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json values_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json objFromPairs_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json pick_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json omit_op(const nlohmann::json& args, ExecutionContext& ctx);

// String operations
nlohmann::json split_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json join_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json trim_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json upper_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json lower_op(const nlohmann::json& args, ExecutionContext& ctx);

// Array operations
nlohmann::json sort_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json reverse_op(const nlohmann::json& args, ExecutionContext& ctx);
nlohmann::json unique_op(const nlohmann::json& args, ExecutionContext& ctx);
}