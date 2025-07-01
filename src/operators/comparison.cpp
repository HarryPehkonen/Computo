#include <computo/operators.hpp>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>
#include <cmath>

namespace computo {
namespace operator_modules {

void init_comparison_operators(std::map<std::string, OperatorFunc>& ops) {
    
    // Greater than operator - supports chaining
    ops[">"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException("> operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<double> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("> operator requires numeric arguments");
            }
            values.push_back(value.get<double>());
        }
        
        // Check chained comparison: a > b > c means a > b && b > c
        for (size_t i = 0; i < values.size() - 1; ++i) {
            if (!(values[i] > values[i + 1])) {
                return false;
            }
        }
        
        return true;
    };
    
    // Less than operator - supports chaining
    ops["<"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException("< operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<double> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("< operator requires numeric arguments");
            }
            values.push_back(value.get<double>());
        }
        
        // Check chained comparison: a < b < c means a < b && b < c
        for (size_t i = 0; i < values.size() - 1; ++i) {
            if (!(values[i] < values[i + 1])) {
                return false;
            }
        }
        
        return true;
    };
    
    // Greater than or equal operator - supports chaining
    ops[">="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException(">= operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<double> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException(">= operator requires numeric arguments");
            }
            values.push_back(value.get<double>());
        }
        
        // Check chained comparison: a >= b >= c means a >= b && b >= c
        for (size_t i = 0; i < values.size() - 1; ++i) {
            if (!(values[i] >= values[i + 1])) {
                return false;
            }
        }
        
        return true;
    };
    
    // Less than or equal operator - supports chaining
    ops["<="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException("<= operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<double> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("<= operator requires numeric arguments");
            }
            values.push_back(value.get<double>());
        }
        
        // Check chained comparison: a <= b <= c means a <= b && b <= c
        for (size_t i = 0; i < values.size() - 1; ++i) {
            if (!(values[i] <= values[i + 1])) {
                return false;
            }
        }
        
        return true;
    };
    
    // Equality operator - supports multiple arguments
    ops["=="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException("== operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<nlohmann::json> values;
        for (const auto& arg : args) {
            values.push_back(evaluate(arg, ctx));
        }
        
        // Check that all values are equal to the first
        const auto& first = values[0];
        for (size_t i = 1; i < values.size(); ++i) {
            if (first != values[i]) {
                return false;
            }
        }
        
        return true;
    };
    
    // Inequality operator
    ops["!="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("!= operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        return left != right;
    };
    
    // Epsilon-based floating point equality
    ops["approx"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 3) {
            throw InvalidArgumentException("approx operator requires exactly 3 arguments: [left, right, epsilon]");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        auto epsilon = evaluate(args[2], ctx);
        
        if (!left.is_number() || !right.is_number() || !epsilon.is_number()) {
            throw InvalidArgumentException("approx operator requires numeric arguments");
        }
        
        double left_val = left.get<double>();
        double right_val = right.get<double>();
        double epsilon_val = epsilon.get<double>();
        
        if (epsilon_val < 0.0) {
            throw InvalidArgumentException("epsilon must be non-negative");
        }
        
        return std::abs(left_val - right_val) <= epsilon_val;
    };
}

} // namespace operator_modules
} // namespace computo 