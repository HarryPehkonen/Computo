#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <initializer_list>

namespace computo {

/**
 * Builder class for constructing Computo JSON expressions with a fluent interface.
 * This eliminates the verbose JSON construction syntax in C++ tests and provides
 * a more readable way to build complex Computo scripts.
 */
class ComputoBuilder {
private:
    nlohmann::json value_;
    
public:
    // Default constructor creates empty builder
    ComputoBuilder() = default;
    
    // Constructor from existing JSON
    explicit ComputoBuilder(const nlohmann::json& val) : value_(val) {}
    
    // Literal value constructors
    static ComputoBuilder literal(const nlohmann::json& val) {
        return ComputoBuilder(val);
    }
    
    static ComputoBuilder number(double val) {
        return ComputoBuilder(val);
    }
    
    static ComputoBuilder string(const std::string& val) {
        return ComputoBuilder(val);
    }
    
    static ComputoBuilder boolean(bool val) {
        return ComputoBuilder(val);
    }
    
    static ComputoBuilder null() {
        return ComputoBuilder(nlohmann::json(nullptr));
    }
    
    // Array construction (using the {"array": [...]} syntax)
    static ComputoBuilder array(std::initializer_list<nlohmann::json> items) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& item : items) {
            arr.push_back(item);
        }
        return ComputoBuilder(nlohmann::json{{"array", arr}});
    }
    
    static ComputoBuilder array(const std::vector<nlohmann::json>& items) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& item : items) {
            arr.push_back(item);
        }
        return ComputoBuilder(nlohmann::json{{"array", arr}});
    }
    
    // Empty array
    static ComputoBuilder empty_array() {
        return ComputoBuilder(nlohmann::json{{"array", nlohmann::json::array()}});
    }
    
    // Object construction using obj operator
    static ComputoBuilder obj() {
        return ComputoBuilder(nlohmann::json::array({"obj"}));
    }
    
    // Add key-value pairs to obj operator
    ComputoBuilder& add_field(const std::string& key, const nlohmann::json& value) {
        if (!value_.is_array() || value_.empty() || value_[0] != "obj") {
            throw std::runtime_error("add_field can only be called on obj() builder");
        }
        value_.push_back(nlohmann::json::array({key, value}));
        return *this;
    }
    
    // Operator construction
    static ComputoBuilder op(const std::string& operator_name) {
        return ComputoBuilder(nlohmann::json::array({operator_name}));
    }
    
    // Add arguments to operator
    ComputoBuilder& arg(const nlohmann::json& argument) {
        if (!value_.is_array()) {
            throw std::runtime_error("arg can only be called on operator builders");
        }
        value_.push_back(argument);
        return *this;
    }
    
    // Add multiple arguments
    ComputoBuilder& args(std::initializer_list<nlohmann::json> arguments) {
        for (const auto& arg : arguments) {
            this->arg(arg);
        }
        return *this;
    }
    
    // Common operators with fluent interface
    static ComputoBuilder add(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"+", a, b}));
    }
    
    static ComputoBuilder add(std::initializer_list<nlohmann::json> args) {
        nlohmann::json result = nlohmann::json::array({"+"});
        for (const auto& arg : args) {
            result.push_back(arg);
        }
        return ComputoBuilder(result);
    }
    
    static ComputoBuilder subtract(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"-", a, b}));
    }
    
    static ComputoBuilder multiply(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"*", a, b}));
    }
    
    static ComputoBuilder divide(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"/", a, b}));
    }
    
    static ComputoBuilder modulo(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"%", a, b}));
    }
    
    // Conditional
    static ComputoBuilder if_then_else(const nlohmann::json& condition, 
                                       const nlohmann::json& then_expr,
                                       const nlohmann::json& else_expr) {
        return ComputoBuilder(nlohmann::json::array({"if", condition, then_expr, else_expr}));
    }
    
    // Variable access
    static ComputoBuilder var(const std::string& name) {
        return ComputoBuilder(nlohmann::json::array({"$", "/" + name}));
    }
    
    // Input access
    static ComputoBuilder input() {
        return ComputoBuilder(nlohmann::json::array({"$input"}));
    }
    
    static ComputoBuilder inputs() {
        return ComputoBuilder(nlohmann::json::array({"$inputs"}));
    }
    
    // String literals for input access (alternative syntax)
    static ComputoBuilder input_str() {
        return ComputoBuilder("$input");
    }
    
    static ComputoBuilder inputs_str() {
        return ComputoBuilder("$inputs");
    }
    
    // Let binding
    static ComputoBuilder let(std::initializer_list<std::pair<std::string, nlohmann::json>> bindings,
                              const nlohmann::json& body) {
        nlohmann::json binding_array = nlohmann::json::array();
        for (const auto& binding : bindings) {
            binding_array.push_back(nlohmann::json::array({binding.first, binding.second}));
        }
        return ComputoBuilder(nlohmann::json::array({"let", binding_array, body}));
    }
    
    // Lambda construction
    static ComputoBuilder lambda(const std::string& param, const nlohmann::json& body) {
        return ComputoBuilder(nlohmann::json::array({"lambda", nlohmann::json::array({param}), body}));
    }
    
    static ComputoBuilder lambda(const std::vector<std::string>& params, const nlohmann::json& body) {
        nlohmann::json param_array = nlohmann::json::array();
        for (const auto& param : params) {
            param_array.push_back(param);
        }
        return ComputoBuilder(nlohmann::json::array({"lambda", param_array, body}));
    }
    
    // Array operations
    static ComputoBuilder map(const nlohmann::json& array_expr, const nlohmann::json& lambda_expr) {
        return ComputoBuilder(nlohmann::json::array({"map", array_expr, lambda_expr}));
    }
    
    static ComputoBuilder filter(const nlohmann::json& array_expr, const nlohmann::json& lambda_expr) {
        return ComputoBuilder(nlohmann::json::array({"filter", array_expr, lambda_expr}));
    }
    
    static ComputoBuilder reduce(const nlohmann::json& array_expr, const nlohmann::json& lambda_expr, const nlohmann::json& initial) {
        return ComputoBuilder(nlohmann::json::array({"reduce", array_expr, lambda_expr, initial}));
    }
    
    // JSON Pointer access
    static ComputoBuilder get(const nlohmann::json& object_expr, const std::string& pointer) {
        return ComputoBuilder(nlohmann::json::array({"get", object_expr, pointer}));
    }
    
    // Comparison operators
    static ComputoBuilder equal(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"==", a, b}));
    }
    
    static ComputoBuilder not_equal(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"!=", a, b}));
    }
    
    static ComputoBuilder less_than(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"<", a, b}));
    }
    
    static ComputoBuilder greater_than(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({">", a, b}));
    }
    
    // Logical operators
    static ComputoBuilder and_(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"and", a, b}));
    }
    
    static ComputoBuilder or_(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"or", a, b}));
    }
    
    static ComputoBuilder not_(const nlohmann::json& a) {
        return ComputoBuilder(nlohmann::json::array({"not", a}));
    }
    
    // Array utility functions
    static ComputoBuilder count(const nlohmann::json& array_expr) {
        return ComputoBuilder(nlohmann::json::array({"count", array_expr}));
    }
    
    static ComputoBuilder find(const nlohmann::json& array_expr, const nlohmann::json& predicate_expr) {
        return ComputoBuilder(nlohmann::json::array({"find", array_expr, predicate_expr}));
    }
    
    static ComputoBuilder some(const nlohmann::json& array_expr, const nlohmann::json& predicate_expr) {
        return ComputoBuilder(nlohmann::json::array({"some", array_expr, predicate_expr}));
    }
    
    static ComputoBuilder every(const nlohmann::json& array_expr, const nlohmann::json& predicate_expr) {
        return ComputoBuilder(nlohmann::json::array({"every", array_expr, predicate_expr}));
    }
    
    static ComputoBuilder partition(const nlohmann::json& array_expr, const nlohmann::json& size_expr) {
        return ComputoBuilder(nlohmann::json::array({"partition", array_expr, size_expr}));
    }
    
    static ComputoBuilder flatmap(const nlohmann::json& array_expr, const nlohmann::json& lambda_expr) {
        return ComputoBuilder(nlohmann::json::array({"flatmap", array_expr, lambda_expr}));
    }
    
    // List operations (Lisp-style)
    static ComputoBuilder car(const nlohmann::json& array_expr) {
        return ComputoBuilder(nlohmann::json::array({"car", array_expr}));
    }
    
    static ComputoBuilder cdr(const nlohmann::json& array_expr) {
        return ComputoBuilder(nlohmann::json::array({"cdr", array_expr}));
    }
    
    static ComputoBuilder cons(const nlohmann::json& item_expr, const nlohmann::json& array_expr) {
        return ComputoBuilder(nlohmann::json::array({"cons", item_expr, array_expr}));
    }
    
    static ComputoBuilder append(const nlohmann::json& array1_expr, const nlohmann::json& array2_expr) {
        return ComputoBuilder(nlohmann::json::array({"append", array1_expr, array2_expr}));
    }
    
    static ComputoBuilder chunk(const nlohmann::json& array_expr, const nlohmann::json& size_expr) {
        return ComputoBuilder(nlohmann::json::array({"chunk", array_expr, size_expr}));
    }
    
    // Extended comparison operators
    static ComputoBuilder less_equal(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({"<=", a, b}));
    }
    
    static ComputoBuilder greater_equal(const nlohmann::json& a, const nlohmann::json& b) {
        return ComputoBuilder(nlohmann::json::array({">=", a, b}));
    }
    
    static ComputoBuilder approx_equal(const nlohmann::json& a, const nlohmann::json& b, const nlohmann::json& tolerance) {
        return ComputoBuilder(nlohmann::json::array({"~=", a, b, tolerance}));
    }
    
    // Merge objects
    static ComputoBuilder merge(std::initializer_list<nlohmann::json> objects) {
        nlohmann::json result = nlohmann::json::array({"merge"});
        for (const auto& obj : objects) {
            result.push_back(obj);
        }
        return ComputoBuilder(result);
    }
    
    // Permuto integration
    static ComputoBuilder permuto_apply(const nlohmann::json& template_expr, const nlohmann::json& context_expr) {
        return ComputoBuilder(nlohmann::json::array({"permuto.apply", template_expr, context_expr}));
    }
    
    // JSON Patch operations
    static ComputoBuilder json_patch_apply(const nlohmann::json& document_expr, const nlohmann::json& patch_expr) {
        return ComputoBuilder(nlohmann::json::array({"json_patch.apply", document_expr, patch_expr}));
    }
    
    static ComputoBuilder json_patch_diff(const nlohmann::json& from_expr, const nlohmann::json& to_expr) {
        return ComputoBuilder(nlohmann::json::array({"json_patch.diff", from_expr, to_expr}));
    }
    
    // Multi-parameter lambda (takes vector of parameter names)
    static ComputoBuilder lambda_multi(const std::vector<std::string>& params, const nlohmann::json& body) {
        nlohmann::json param_array = nlohmann::json::array();
        for (const auto& param : params) {
            param_array.push_back(param);
        }
        return ComputoBuilder(nlohmann::json::array({"lambda", param_array, body}));
    }
    
    // Advanced array operations
    static ComputoBuilder zip(const nlohmann::json& array1_expr, const nlohmann::json& array2_expr) {
        return ComputoBuilder(nlohmann::json::array({"zip", array1_expr, array2_expr}));
    }
    
    static ComputoBuilder zip_with(const nlohmann::json& lambda_expr, const nlohmann::json& array1_expr, const nlohmann::json& array2_expr) {
        return ComputoBuilder(nlohmann::json::array({"zipWith", lambda_expr, array1_expr, array2_expr}));
    }
    
    static ComputoBuilder enumerate(const nlohmann::json& array_expr) {
        return ComputoBuilder(nlohmann::json::array({"enumerate", array_expr}));
    }
    
    static ComputoBuilder map_with_index(const nlohmann::json& array_expr, const nlohmann::json& lambda_expr) {
        return ComputoBuilder(nlohmann::json::array({"mapWithIndex", array_expr, lambda_expr}));
    }
    
    static ComputoBuilder concat(std::initializer_list<nlohmann::json> arrays) {
        nlohmann::json result = nlohmann::json::array({"concat"});
        for (const auto& arr : arrays) {
            result.push_back(arr);
        }
        return ComputoBuilder(result);
    }
    
    // Build the final JSON
    nlohmann::json build() const {
        return value_;
    }
    
    // Implicit conversion to json for convenience
    operator nlohmann::json() const {
        return value_;
    }
    
    // Allow chaining with other builders
    ComputoBuilder& operator<<(const ComputoBuilder& other) {
        return arg(other.build());
    }
    
    ComputoBuilder& operator<<(const nlohmann::json& json_val) {
        return arg(json_val);
    }
};

// Convenience aliases for common patterns
using CB = ComputoBuilder;  // Short alias for tests

} // namespace computo