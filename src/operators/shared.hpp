#pragma once

#include <computo.hpp>
#include <vector>
#include <string>

namespace computo {

// --- Shared Operator Utilities ---

/**
 * Determine if a JSON value is truthy according to Computo semantics
 * - false, null, 0, "", empty arrays/objects are falsy
 * - Everything else is truthy
 */
auto is_truthy(const jsom::JsonDocument& value) -> bool;

/**
 * Validate that all arguments are numeric (int or double)
 * Throws InvalidArgumentException if any argument is not numeric
 */
auto validate_numeric_args(const jsom::JsonDocument& args, const std::string& op_name, const std::string& path) -> void;

/**
 * Evaluate a lambda expression (array starting with arguments list)
 * Used by map, filter, reduce, etc.
 * 
 * Lambda format: [["arg1", "arg2"], body_expression]
 * 
 * @param lambda_expr The lambda expression to evaluate
 * @param lambda_args The arguments to bind to the lambda parameters
 * @param ctx The execution context
 * @return The result of evaluating the lambda body
 */
auto evaluate_lambda(const jsom::JsonDocument& lambda_expr, 
                     const std::vector<jsom::JsonDocument>& lambda_args,
                     ExecutionContext& ctx) -> EvaluationResult;

/**
 * Convert a JSON value to a numeric double
 * Throws InvalidArgumentException if the value is not numeric
 */
auto to_numeric(const jsom::JsonDocument& value, const std::string& op_name, const std::string& path) -> double;

/**
 * Get the type name of a JSON value for error messages
 */
auto get_type_name(const jsom::JsonDocument& value) -> std::string;

/**
 * Extract array data from either {"array": [...]} format or direct array format
 * This is a common pattern across many array operators
 * 
 * @param array_input The input that may be {"array": [...]} or [...]
 * @param op_name The operator name for error messages
 * @param path The execution path for error messages
 * @param array_key The custom array wrapper key (defaults to "array")
 * @return The actual array data
 */
auto extract_array_data(const jsom::JsonDocument& array_input, 
                        const std::string& op_name, 
                        const std::string& path,
                        const std::string& array_key = "array") -> jsom::JsonDocument;

/**
 * Calculate Levenshtein distance between two strings
 * Used for typo detection in operator and variable names
 * 
 * @param first_string First string
 * @param second_string Second string
 * @return Edit distance between the strings
 */
auto calculate_levenshtein_distance(const std::string& first_string, const std::string& second_string) -> int;

/**
 * Find similar names using Levenshtein distance
 * Returns candidates within max_distance edits, sorted by distance
 * 
 * @param target The misspelled name to find suggestions for
 * @param candidates List of valid names to search
 * @param max_distance Maximum edit distance to consider (default: 2)
 * @return Vector of suggested names, sorted by similarity
 */
auto suggest_similar_names(const std::string& target, 
                          const std::vector<std::string>& candidates, 
                          int max_distance = 2) -> std::vector<std::string>;

/**
 * A generic function to process an array with a per-item lambda
 * This consolidates the common boilerplate shared by map, filter, find, some, every
 * 
 * @param args The operator arguments (array, lambda)
 * @param ctx The execution context
 * @param op_name The operator name for error messages  
 * @param processor A callback that processes each (item, lambda_result) pair and can modify final_result
 * @return The processor's populated result
 */
auto process_array_with_lambda(const jsom::JsonDocument& args, ExecutionContext& ctx, const std::string& op_name,
                               const std::function<bool(const jsom::JsonDocument& item, const jsom::JsonDocument& lambda_result, jsom::JsonDocument& final_result)>& processor) -> jsom::JsonDocument;

/**
 * Evaluate a JSON Pointer path against a JSON object
 * Centralizes JSON Pointer validation and error handling
 * 
 * @param root The JSON object to evaluate the pointer against
 * @param pointer_str The JSON Pointer string (must start with '/')
 * @param path_context The execution path context for error messages
 * @return The value at the pointer location
 * @throws InvalidArgumentException if pointer is invalid or path not found
 */
auto evaluate_json_pointer(const jsom::JsonDocument& root, const std::string& pointer_str, const std::string& path_context) -> jsom::JsonDocument;

/**
 * Parse a variable path like "/varname/sub/path" into variable name and sub-path
 * Used by the $ operator to separate variable lookup from sub-path evaluation
 * 
 * @param full_path The full path starting with '/' (e.g., "/var/field")
 * @return Pair of {variable_name, sub_path} where sub_path may be empty
 */
struct VariablePathParts {
    std::string variable_name;
    std::string sub_path;
};
auto parse_variable_path(const std::string& full_path) -> VariablePathParts;

} // namespace computo
