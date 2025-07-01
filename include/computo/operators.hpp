#pragma once

#include <map>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace computo {

// Forward declaration
class ExecutionContext;

// Type alias for operator functions
using OperatorFunc = std::function<nlohmann::json(const nlohmann::json&, ExecutionContext&)>;

namespace operator_modules {

// Arithmetic operators: +, -, *, /, %
void init_arithmetic_operators(std::map<std::string, OperatorFunc>& ops);

// Logical operators: &&, ||, not
void init_logical_operators(std::map<std::string, OperatorFunc>& ops);

// Comparison operators: ==, !=, <, >, <=, >=, approx
void init_comparison_operators(std::map<std::string, OperatorFunc>& ops);

// Array operators: map, filter, reduce, find, some, every, flatMap, etc.
void init_array_operators(std::map<std::string, OperatorFunc>& ops);

// Utility operators: $, get, obj, permuto.apply, diff, patch, etc.
void init_utility_operators(std::map<std::string, OperatorFunc>& ops);

// List operations: car, cdr, cons, append, chunk, partition, etc.
void init_list_operators(std::map<std::string, OperatorFunc>& ops);

// String and type conversion operators
void init_string_operators(std::map<std::string, OperatorFunc>& ops);

} // namespace operator_modules
} // namespace computo 