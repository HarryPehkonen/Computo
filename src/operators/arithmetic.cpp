#include <computo/operators.hpp>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

namespace computo {
namespace operator_modules {

void init_arithmetic_operators(std::map<std::string, OperatorFunc>& ops) {
    
    // Addition operator - n-ary
    ops["+"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 1) {
            throw InvalidArgumentException("+ operator requires at least 1 argument");
        }
        
        // Evaluate all arguments first
        std::vector<nlohmann::json> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("+ operator requires numeric arguments");
            }
            values.push_back(value);
        }
        
        // Start with first value
        bool all_integers = values[0].is_number_integer();
        int64_t int_result = all_integers ? values[0].get<int64_t>() : 0;
        double double_result = values[0].get<double>();
        
        // Add remaining values
        for (size_t i = 1; i < values.size(); ++i) {
            if (all_integers && values[i].is_number_integer()) {
                int_result += values[i].get<int64_t>();
                double_result += values[i].get<double>();
            } else {
                all_integers = false;
                double_result += values[i].get<double>();
            }
        }
        
        return all_integers ? nlohmann::json(int_result) : nlohmann::json(double_result);
    };
    
    // Subtraction operator
    ops["-"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("- operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("- operator requires numeric arguments");
        }
        
        if (left.is_number_integer() && right.is_number_integer()) {
            return left.get<int64_t>() - right.get<int64_t>();
        } else {
            return left.get<double>() - right.get<double>();
        }
    };
    
    // Multiplication operator - n-ary
    ops["*"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 1) {
            throw InvalidArgumentException("* operator requires at least 1 argument");
        }
        
        // Evaluate all arguments first
        std::vector<nlohmann::json> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("* operator requires numeric arguments");
            }
            values.push_back(value);
        }
        
        // Start with first value
        bool all_integers = values[0].is_number_integer();
        int64_t int_result = all_integers ? values[0].get<int64_t>() : 0;
        double double_result = values[0].get<double>();
        
        // Multiply remaining values
        for (size_t i = 1; i < values.size(); ++i) {
            if (all_integers && values[i].is_number_integer()) {
                int_result *= values[i].get<int64_t>();
                double_result *= values[i].get<double>();
            } else {
                all_integers = false;
                double_result *= values[i].get<double>();
            }
        }
        
        return all_integers ? nlohmann::json(int_result) : nlohmann::json(double_result);
    };
    
    // Division operator
    ops["/"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("/ operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("/ operator requires numeric arguments");
        }
        
        double right_val = right.get<double>();
        if (right_val == 0.0) {
            throw InvalidArgumentException("Division by zero");
        }
        
        return left.get<double>() / right_val;
    };
    
    // Modulo operator
    ops["%"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("% operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("% operator requires numeric arguments");
        }
        
        // For modulo, we need integers
        if (!left.is_number_integer() || !right.is_number_integer()) {
            throw InvalidArgumentException("% operator requires integer arguments");
        }
        
        int64_t right_val = right.get<int64_t>();
        if (right_val == 0) {
            throw InvalidArgumentException("Modulo by zero");
        }
        
        return left.get<int64_t>() % right_val;
    };
}

} // namespace operator_modules
} // namespace computo 