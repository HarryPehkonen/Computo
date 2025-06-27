#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ComputoTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test literal values
TEST_F(ComputoTest, LiteralValues) {
    // Test numbers
    EXPECT_EQ(computo::execute(42, input_data), 42);
    EXPECT_EQ(computo::execute(3.14, input_data), 3.14);
    
    // Test strings
    EXPECT_EQ(computo::execute("hello", input_data), "hello");
    
    // Test booleans
    EXPECT_EQ(computo::execute(true, input_data), true);
    EXPECT_EQ(computo::execute(false, input_data), false);
    
    // Test null
    EXPECT_EQ(computo::execute(json(nullptr), input_data), json(nullptr));
    
    // Test arrays as literals using new syntax
    json arr_literal = json{{"array", json::array({1, 2, 3})}};
    json expected_arr = json::array({1, 2, 3});
    EXPECT_EQ(computo::execute(arr_literal, input_data), expected_arr);
    
    // Test objects as literals
    json obj = json{{"key", "value"}};
    EXPECT_EQ(computo::execute(obj, input_data), obj);
}

// Test $input operator
TEST_F(ComputoTest, InputOperator) {
    json script = json::array({"$input"});
    EXPECT_EQ(computo::execute(script, input_data), input_data);
}

// Test addition operator
TEST_F(ComputoTest, AdditionOperator) {
    // Test integer addition
    json script1 = json::array({"+", 2, 3});
    EXPECT_EQ(computo::execute(script1, input_data), 5);
    
    // Test floating point addition
    json script2 = json::array({"+", 2.5, 1.5});
    EXPECT_EQ(computo::execute(script2, input_data), 4.0);
    
    // Test mixed integer/float addition
    json script3 = json::array({"+", 2, 1.5});
    EXPECT_EQ(computo::execute(script3, input_data), 3.5);
    
    // Test nested addition
    json script4 = json::array({"+", json::array({"+", 1, 2}), 3});
    EXPECT_EQ(computo::execute(script4, input_data), 6);
}

// Test addition with no arguments
TEST_F(ComputoTest, AdditionNoArguments) {
    json script = json::array({"+"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with one argument
TEST_F(ComputoTest, AdditionOneArgument) {
    json script = json::array({"+", 1});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with too many arguments
TEST_F(ComputoTest, AdditionTooManyArguments) {
    json script = json::array({"+", 1, 2, 3});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with string argument
TEST_F(ComputoTest, AdditionStringArgument) {
    json script = json::array({"+", "hello", 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with boolean argument
TEST_F(ComputoTest, AdditionBooleanArgument) {
    json script = json::array({"+", 1, true});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test invalid operator
TEST_F(ComputoTest, InvalidOperator) {
    json script = json::array({"unknown_operator", 1, 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidOperatorException);
}

// Test array objects vs operator calls
TEST_F(ComputoTest, ArrayObjectsVsOperatorCalls) {
    // Array object should create literal array
    json array_obj = json{{"array", json::array({1, 2, 3})}};
    json expected_array = json::array({1, 2, 3});
    EXPECT_EQ(computo::execute(array_obj, input_data), expected_array);
    
    // Empty array object
    json empty_array_obj = json{{"array", json::array()}};
    json expected_empty = json::array();
    EXPECT_EQ(computo::execute(empty_array_obj, input_data), expected_empty);
    
    // Arrays starting with numbers should now throw errors (no longer literals)
    json invalid_script = json::array({1, 2, 3});
    EXPECT_THROW(computo::execute(invalid_script, input_data), computo::InvalidArgumentException);
}

// Test exception hierarchy - InvalidArgumentException inherits from ComputoException
TEST_F(ComputoTest, InvalidArgumentExceptionHierarchy) {
    json script = json::array({"+", "hello", 2});
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException to be thrown";
    } catch (const computo::ComputoException& e) {
        // This should catch InvalidArgumentException since it inherits from ComputoException
        SUCCEED();
    } catch (...) {
        FAIL() << "InvalidArgumentException should inherit from ComputoException";
    }
}

// Test exception hierarchy - InvalidOperatorException inherits from ComputoException
TEST_F(ComputoTest, InvalidOperatorExceptionHierarchy) {
    json script = json::array({"unknown_operator", 1, 2});
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidOperatorException to be thrown";
    } catch (const computo::ComputoException& e) {
        // This should catch InvalidOperatorException since it inherits from ComputoException
        SUCCEED();
    } catch (...) {
        FAIL() << "InvalidOperatorException should inherit from ComputoException";
    }
}

// Test exception hierarchy - All exceptions inherit from std::exception
TEST_F(ComputoTest, StdExceptionHierarchy) {
    json script = json::array({"+", 1, true});
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException to be thrown";
    } catch (const std::exception& e) {
        // This should catch any of our exceptions since they all inherit from std::exception
        SUCCEED();
    } catch (...) {
        FAIL() << "All Computo exceptions should inherit from std::exception";
    }
}

// === Phase 2 Tests: State and Data Access ===

// Test let operator basic functionality
TEST_F(ComputoTest, LetOperatorBasic) {
    // ["let", [["x", 5]], ["$", "/x"]]
    json script = json::array({
        "let",
        json::array({json::array({"x", 5})}),
        json::array({"$", "/x"})
    });
    EXPECT_EQ(computo::execute(script, input_data), 5);
}

// Test let operator with multiple variables
TEST_F(ComputoTest, LetOperatorMultipleVars) {
    // ["let", [["x", 5], ["y", 10]], ["+", ["$", "/x"], ["$", "/y"]]]
    json script = json::array({
        "let",
        json::array({
            json::array({"x", 5}),
            json::array({"y", 10})
        }),
        json::array({"+", json::array({"$", "/x"}), json::array({"$", "/y"})})
    });
    EXPECT_EQ(computo::execute(script, input_data), 15);
}

// Test let operator with expression evaluation in bindings
TEST_F(ComputoTest, LetOperatorExpressionBinding) {
    // ["let", [["x", ["+", 2, 3]]], ["$", "/x"]]
    json script = json::array({
        "let",
        json::array({json::array({"x", json::array({"+", 2, 3})})}),
        json::array({"$", "/x"})
    });
    EXPECT_EQ(computo::execute(script, input_data), 5);
}

// Test let operator variable shadowing
TEST_F(ComputoTest, LetOperatorShadowing) {
    // ["let", [["x", 5]], ["let", [["x", 10]], ["$", "/x"]]]
    json script = json::array({
        "let",
        json::array({json::array({"x", 5})}),
        json::array({
            "let",
            json::array({json::array({"x", 10})}),
            json::array({"$", "/x"})
        })
    });
    EXPECT_EQ(computo::execute(script, input_data), 10);
}

// Test $ operator variable lookup
TEST_F(ComputoTest, DollarOperatorLookup) {
    // Create context with a variable manually (for isolated testing)
    computo::ExecutionContext ctx(input_data);
    ctx.variables["test_var"] = 42;
    
    json script = json::array({"$", "/test_var"});
    EXPECT_EQ(computo::evaluate(script, ctx), 42);
}

// Test get operator with simple object access
TEST_F(ComputoTest, GetOperatorSimple) {
    // ["get", {"name": "John", "age": 30}, "/name"]
    json script = json::array({
        "get",
        json{{"name", "John"}, {"age", 30}},
        "/name"
    });
    EXPECT_EQ(computo::execute(script, input_data), "John");
}

// Test get operator with nested object access
TEST_F(ComputoTest, GetOperatorNested) {
    // ["get", {"user": {"profile": {"name": "Alice"}}}, "/user/profile/name"]
    json script = json::array({
        "get",
        json{{"user", json{{"profile", json{{"name", "Alice"}}}}}},
        "/user/profile/name"
    });
    EXPECT_EQ(computo::execute(script, input_data), "Alice");
}

// Test get operator with array access
TEST_F(ComputoTest, GetOperatorArray) {
    // ["get", {"array": [10, 20, 30]}, "/1"]
    json script = json::array({
        "get",
        json{{"array", json::array({10, 20, 30})}},
        "/1"
    });
    EXPECT_EQ(computo::execute(script, input_data), 20);
}

// Test get operator with $input
TEST_F(ComputoTest, GetOperatorWithInput) {
    json test_input = json{{"user", json{{"id", 123}}}};
    // ["get", ["$input"], "/user/id"]
    json script = json::array({
        "get",
        json::array({"$input"}),
        "/user/id"
    });
    EXPECT_EQ(computo::execute(script, test_input), 123);
}

// Test complex combination: let + $ + get
TEST_F(ComputoTest, LetDollarGetCombination) {
    json test_input = json{{"data", json{{"values", json::array({1, 2, 3})}}}};
    
    // ["let", [["obj", ["$input"]]], ["get", ["$", "/obj"], "/data/values/2"]]
    json script = json::array({
        "let",
        json::array({json::array({"obj", json::array({"$input"})})}),
        json::array({
            "get",
            json::array({"$", "/obj"}),
            "/data/values/2"
        })
    });
    EXPECT_EQ(computo::execute(script, test_input), 3);
}

// === Error Tests for Phase 2 ===

// Test let operator errors
TEST_F(ComputoTest, LetOperatorWrongArgCount) {
    json script = json::array({"let", json::array({json::array({"x", 5})})});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, LetOperatorNonArrayBindings) {
    json script = json::array({"let", "not_array", json::array({"$", "/x"})});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, LetOperatorInvalidBinding) {
    json script = json::array({
        "let",
        json::array({json::array({"x"})}), // Missing value
        json::array({"$", "/x"})
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, LetOperatorNonStringVarName) {
    json script = json::array({
        "let",
        json::array({json::array({123, 5})}), // Number instead of string
        json::array({"$", "/x"})
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test $ operator errors
TEST_F(ComputoTest, DollarOperatorWrongArgCount) {
    json script = json::array({"$"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, DollarOperatorNonStringPath) {
    json script = json::array({"$", 123});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, DollarOperatorInvalidPath) {
    json script = json::array({"$", "no_leading_slash"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, DollarOperatorVariableNotFound) {
    json script = json::array({"$", "/nonexistent"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test get operator errors
TEST_F(ComputoTest, GetOperatorWrongArgCount) {
    json script = json::array({"get", json{{"key", "value"}}});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, GetOperatorNonStringPointer) {
    json script = json::array({"get", json{{"key", "value"}}, 123});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, GetOperatorInvalidPointer) {
    json script = json::array({"get", json{{"key", "value"}}, "/nonexistent"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// === Phase 3 Tests: Logic and Construction ===

// Test if operator with boolean conditions
TEST_F(ComputoTest, IfOperatorBoolean) {
    // ["if", true, "yes", "no"]
    json script1 = json::array({"if", true, "yes", "no"});
    EXPECT_EQ(computo::execute(script1, input_data), "yes");
    
    // ["if", false, "yes", "no"]
    json script2 = json::array({"if", false, "yes", "no"});
    EXPECT_EQ(computo::execute(script2, input_data), "no");
}

// Test if operator with numeric conditions
TEST_F(ComputoTest, IfOperatorNumeric) {
    // ["if", 1, "truthy", "falsy"] - non-zero is true
    json script1 = json::array({"if", 1, "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script1, input_data), "truthy");
    
    // ["if", 0, "truthy", "falsy"] - zero is false
    json script2 = json::array({"if", 0, "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script2, input_data), "falsy");
    
    // ["if", 3.14, "truthy", "falsy"] - non-zero float is true
    json script3 = json::array({"if", 3.14, "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script3, input_data), "truthy");
    
    // ["if", 0.0, "truthy", "falsy"] - zero float is false
    json script4 = json::array({"if", 0.0, "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script4, input_data), "falsy");
}

// Test if operator with string conditions
TEST_F(ComputoTest, IfOperatorString) {
    // ["if", "hello", "truthy", "falsy"] - non-empty string is true
    json script1 = json::array({"if", "hello", "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script1, input_data), "truthy");
    
    // ["if", "", "truthy", "falsy"] - empty string is false
    json script2 = json::array({"if", "", "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script2, input_data), "falsy");
}

// Test if operator with null condition
TEST_F(ComputoTest, IfOperatorNull) {
    // ["if", null, "truthy", "falsy"] - null is false
    json script = json::array({"if", json(nullptr), "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script, input_data), "falsy");
}

// Test if operator with array conditions
TEST_F(ComputoTest, IfOperatorArray) {
    // ["if", {"array": [1, 2]}, "truthy", "falsy"] - non-empty array is true
    json script1 = json::array({"if", json{{"array", json::array({1, 2})}}, "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script1, input_data), "truthy");
    
    // ["if", {"array": []}, "truthy", "falsy"] - empty array is false
    json script2 = json::array({"if", json{{"array", json::array()}}, "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script2, input_data), "falsy");
}

// Test if operator with object conditions
TEST_F(ComputoTest, IfOperatorObject) {
    // ["if", {"key": "value"}, "truthy", "falsy"] - non-empty object is true
    json script1 = json::array({"if", json{{"key", "value"}}, "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script1, input_data), "truthy");
    
    // ["if", {}, "truthy", "falsy"] - empty object is false
    json script2 = json::array({"if", json::object(), "truthy", "falsy"});
    EXPECT_EQ(computo::execute(script2, input_data), "falsy");
}

// Test if operator with expression evaluation
TEST_F(ComputoTest, IfOperatorWithExpressions) {
    // ["if", ["+", 1, 1], ["+", 10, 5], ["+", 20, 5]]
    json script = json::array({
        "if", 
        json::array({"+", 1, 1}), // condition evaluates to 2 (truthy)
        json::array({"+", 10, 5}), // then branch
        json::array({"+", 20, 5})  // else branch
    });
    EXPECT_EQ(computo::execute(script, input_data), 15);
}

// Test obj operator basic functionality
TEST_F(ComputoTest, ObjOperatorBasic) {
    // ["obj", ["name", "John"], ["age", 30]]
    json script = json::array({
        "obj",
        json::array({"name", "John"}),
        json::array({"age", 30})
    });
    json expected = json{{"name", "John"}, {"age", 30}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test obj operator with expression values
TEST_F(ComputoTest, ObjOperatorWithExpressions) {
    // ["obj", ["sum", ["+", 2, 3]], ["input", ["$input"]]]
    json script = json::array({
        "obj",
        json::array({"sum", json::array({"+", 2, 3})}),
        json::array({"input", json::array({"$input"})})
    });
    json expected = json{{"sum", 5}, {"input", input_data}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test obj operator empty object
TEST_F(ComputoTest, ObjOperatorEmpty) {
    // ["obj"] - no key-value pairs
    json script = json::array({"obj"});
    json expected = json::object();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test array object basic functionality
TEST_F(ComputoTest, ArrayObjectBasic) {
    // {"array": [1, "hello", true]}
    json script = json{{"array", json::array({1, "hello", true})}};
    json expected = json::array({1, "hello", true});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test array object with expressions
TEST_F(ComputoTest, ArrayObjectWithExpressions) {
    // {"array": [["+", 1, 2], ["$input"], ["obj", ["key", "value"]]]}
    json script = json{{"array", json::array({
        json::array({"+", 1, 2}),
        json::array({"$input"}),
        json::array({"obj", json::array({"key", "value"})})
    })}};
    json expected = json::array({3, input_data, json{{"key", "value"}}});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test array object empty array  
TEST_F(ComputoTest, ArrayObjectEmpty) {
    // {"array": []} - no elements
    json script = json{{"array", json::array()}};
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test complex combination: if + obj + array
TEST_F(ComputoTest, IfObjArrayCombination) {
    // ["if", true, ["obj", ["list", {"array": [1, 2, 3]}]], {"array": []}]
    json script = json::array({
        "if", 
        true,
        json::array({
            "obj",
            json::array({"list", json{{"array", json::array({1, 2, 3})}}})
        }),
        json{{"array", json::array()}}
    });
    json expected = json{{"list", json::array({1, 2, 3})}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test nested if statements
TEST_F(ComputoTest, NestedIfStatements) {
    // ["if", true, ["if", false, "inner-true", "inner-false"], "outer-false"]
    json script = json::array({
        "if", 
        true,
        json::array({"if", false, "inner-true", "inner-false"}),
        "outer-false"
    });
    EXPECT_EQ(computo::execute(script, input_data), "inner-false");
}

// === Error Tests for Phase 3 ===

// Test if operator errors
TEST_F(ComputoTest, IfOperatorWrongArgCount) {
    json script1 = json::array({"if", true});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"if", true, "then"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    json script3 = json::array({"if", true, "then", "else", "extra"});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// Test obj operator errors
TEST_F(ComputoTest, ObjOperatorInvalidPair) {
    // Single element instead of pair
    json script1 = json::array({"obj", json::array({"key"})});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Three elements instead of pair
    json script2 = json::array({"obj", json::array({"key", "value", "extra"})});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Non-array pair
    json script3 = json::array({"obj", "not_array"});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, ObjOperatorNonStringKey) {
    // Numeric key instead of string
    json script = json::array({"obj", json::array({123, "value"})});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// === Phase 4 Tests: The Permuto Bridge ===

// Test permuto.apply basic functionality
TEST_F(ComputoTest, PermutoApplyBasic) {
    json template_json = json{{"greeting", "${/name}"}};
    json context = json{{"name", "World"}};
    
    // ["permuto.apply", template, context]
    json script = json::array({"permuto.apply", template_json, context});
    json expected = json{{"greeting", "World"}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test permuto.apply with expressions
TEST_F(ComputoTest, PermutoApplyWithExpressions) {
    json template_json = json{{"user", "${/user/name}"}, {"count", "${/items/length}"}};
    
    // Build context using Computo operators
    json script = json::array({
        "permuto.apply",
        template_json,
        json::array({
            "obj",
            json::array({"user", json::array({"obj", json::array({"name", "Alice"})})}),
            json::array({"items", json::array({"obj", json::array({"length", 5})})})
        })
    });
    
    json expected = json{{"user", "Alice"}, {"count", 5}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test permuto.apply with input data
TEST_F(ComputoTest, PermutoApplyWithInput) {
    json test_input = json{{"user", json{{"id", 123}, {"name", "Bob"}}}};
    json template_json = json{{"user_id", "${/user/id}"}, {"greeting", "Hello ${/user/name}!"}};
    
    // ["permuto.apply", template, ["$input"]]
    json script = json::array({
        "permuto.apply",
        template_json,
        json::array({"$input"})
    });
    
    // Create permuto options with interpolation enabled
    permuto::Options opts;
    opts.enable_interpolation = true;
    
    json expected = json{{"user_id", 123}, {"greeting", "Hello Bob!"}};
    EXPECT_EQ(computo::execute(script, test_input, opts), expected);
}

// Test permuto.apply with variables from let
TEST_F(ComputoTest, PermutoApplyWithLet) {
    json template_json = json{{"message", "${/msg}"}, {"value", "${/val}"}};
    
    // ["let", [["data", ["obj", ["msg", "hello"], ["val", 42]]]], ["permuto.apply", template, ["$", "/data"]]]
    json script = json::array({
        "let",
        json::array({
            json::array({
                "data",
                json::array({
                    "obj",
                    json::array({"msg", "hello"}),
                    json::array({"val", 42})
                })
            })
        }),
        json::array({
            "permuto.apply",
            template_json,
            json::array({"$", "/data"})
        })
    });
    
    json expected = json{{"message", "hello"}, {"value", 42}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test permuto.apply with complex template
TEST_F(ComputoTest, PermutoApplyComplexTemplate) {
    json template_json = json{
        {"type", "user_profile"},
        {"data", json{
            {"name", "${/user/name}"},
            {"settings", json{
                {"theme", "${/user/preferences/theme}"},
                {"notifications", "${/user/preferences/notifications}"}
            }}
        }},
        {"metadata", json{
            {"timestamp", "${/timestamp}"},
            {"version", "1.0"}
        }}
    };
    
    json test_input = json{
        {"user", json{
            {"name", "Alice"},
            {"preferences", json{
                {"theme", "dark"},
                {"notifications", true}
            }}
        }},
        {"timestamp", 1234567890}
    };
    
    json script = json::array({
        "permuto.apply",
        template_json,
        json::array({"$input"})
    });
    
    json expected = json{
        {"type", "user_profile"},
        {"data", json{
            {"name", "Alice"},
            {"settings", json{
                {"theme", "dark"},
                {"notifications", true}
            }}
        }},
        {"metadata", json{
            {"timestamp", 1234567890},
            {"version", "1.0"}
        }}
    };
    
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test permuto.apply with conditional logic
TEST_F(ComputoTest, PermutoApplyConditional) {
    json test_input = json{{"user", json{{"active", true}, {"name", "Charlie"}}}};
    
    // Use if to choose template based on user status
    json script = json::array({
        "if",
        json::array({"get", json::array({"$input"}), "/user/active"}),
        // Active user template
        json::array({
            "permuto.apply",
            json{{"status", "active"}, {"greeting", "Welcome ${/user/name}!"}},
            json::array({"$input"})
        }),
        // Inactive user template  
        json::array({
            "permuto.apply",
            json{{"status", "inactive"}, {"message", "Account suspended"}},
            json::array({"$input"})
        })
    });
    
    permuto::Options opts;
    opts.enable_interpolation = true;
    
    json expected = json{{"status", "active"}, {"greeting", "Welcome Charlie!"}};
    EXPECT_EQ(computo::execute(script, test_input, opts), expected);
}

// === Error Tests for Phase 4 ===

// Test permuto.apply errors
TEST_F(ComputoTest, PermutoApplyWrongArgCount) {
    json script1 = json::array({"permuto.apply", json{{"key", "${/value}"}}});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"permuto.apply", json{{"key", "${/value}"}}, json{{"value", "test"}}, "extra"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test permuto.apply with invalid template (missing key)
TEST_F(ComputoTest, PermutoApplyMissingKey) {
    json template_json = json{{"value", "${/nonexistent}"}};
    json context = json{{"other", "value"}};
    
    json script = json::array({"permuto.apply", template_json, context});
    
    // Default behavior should be to ignore missing keys (leave as-is)
    json expected = json{{"value", "${/nonexistent}"}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test permuto.apply with error behavior for missing keys
TEST_F(ComputoTest, PermutoApplyMissingKeyError) {
    json template_json = json{{"value", "${/nonexistent}"}};
    json context = json{{"other", "value"}};
    
    json script = json::array({"permuto.apply", template_json, context});
    
    // Configure to error on missing keys
    permuto::Options opts;
    opts.missing_key_behavior = permuto::MissingKeyBehavior::Error;
    
    EXPECT_THROW(computo::execute(script, input_data, opts), computo::InvalidArgumentException);
}

// === Phase 5 Tests: Iteration ===

// Test map operator basic functionality
TEST_F(ComputoTest, MapOperatorBasic) {
    // ["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["+", ["$", "/x"], 1]]]
    json script = json::array({
        "map",
        json{{"array", json::array({1, 2, 3})}},
        json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})
    });
    json expected = json::array({2, 3, 4});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test map operator with string transformation
TEST_F(ComputoTest, MapOperatorStringTransform) {
    // ["map", {"array": ["hello", "world"]}, ["lambda", ["s"], ["$", "/s"]]]
    json script = json::array({
        "map",
        json{{"array", json::array({"hello", "world"})}},
        json::array({"lambda", json::array({"s"}), json::array({"$", "/s"})})
    });
    json expected = json::array({"hello", "world"});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test map operator with complex lambda
TEST_F(ComputoTest, MapOperatorComplexLambda) {
    json array_data = json{{"array", json::array({
        json{{"id", 1}, {"value", 10}},
        json{{"id", 2}, {"value", 20}},
        json{{"id", 3}, {"value", 30}}
    })}};
    
    // ["map", {"array": [...]}, ["lambda", ["item"], ["obj", ["id", ["get", ["$", "/item"], "/id"]], ["doubled", ["+", ["get", ["$", "/item"], "/value"], ["get", ["$", "/item"], "/value"]]]]]]
    json script = json::array({
        "map",
        array_data,
        json::array({
            "lambda",
            json::array({"item"}),
            json::array({
                "obj",
                json::array({"id", json::array({"get", json::array({"$", "/item"}), "/id"})}),
                json::array({"doubled", json::array({"+", json::array({"get", json::array({"$", "/item"}), "/value"}), json::array({"get", json::array({"$", "/item"}), "/value"})})})
            })
        })
    });
    
    json expected = json::array({
        json{{"id", 1}, {"doubled", 20}},
        json{{"id", 2}, {"doubled", 40}},
        json{{"id", 3}, {"doubled", 60}}
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test map operator with empty array
TEST_F(ComputoTest, MapOperatorEmpty) {
    json script = json::array({
        "map",
        json::array(),
        json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})
    });
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator basic functionality
TEST_F(ComputoTest, FilterOperatorBasic) {
    // ["filter", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], ["$", "/x"]]]
    json script = json::array({
        "filter",
        json{{"array", json::array({1, 2, 3, 4, 5})}},
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})}) // Filter out 0 (falsy)
    });
    // All numbers are truthy, so all should pass
    json expected = json::array({1, 2, 3, 4, 5});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator with boolean condition
TEST_F(ComputoTest, FilterOperatorBoolean) {
    // ["filter", {"array": [true, false, true, false]}, ["lambda", ["x"], ["$", "/x"]]]
    json script = json::array({
        "filter",
        json{{"array", json::array({true, false, true, false})}},
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    json expected = json::array({true, true});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator with string condition - TEMPORARILY DISABLED
// Issue: strings being treated as operators - needs investigation
TEST_F(ComputoTest, FilterOperatorString) {
    // ["filter", {"array": ["hello", "", "world", ""]}, ["lambda", ["s"], ["$", "/s"]]]
    json script = json::array({
        "filter",
        json{{"array", json::array({"hello", "", "world", ""})}},
        json::array({"lambda", json::array({"s"}), json::array({"$", "/s"})})
    });
    json expected = json::array({"hello", "world"});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator with object condition
TEST_F(ComputoTest, FilterOperatorObject) {
    json array_data = json{{"array", json::array({
        json{{"active", true}, {"name", "Alice"}},
        json{{"active", false}, {"name", "Bob"}},
        json{{"active", true}, {"name", "Charlie"}}
    })}};
    
    // ["filter", {"array": [...]}, ["lambda", ["user"], ["get", ["$", "/user"], "/active"]]]
    json script = json::array({
        "filter",
        array_data,
        json::array({
            "lambda",
            json::array({"user"}),
            json::array({"get", json::array({"$", "/user"}), "/active"})
        })
    });
    
    json expected = json::array({
        json{{"active", true}, {"name", "Alice"}},
        json{{"active", true}, {"name", "Charlie"}}
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator with empty array
TEST_F(ComputoTest, FilterOperatorEmpty) {
    json script = json::array({
        "filter",
        json::array(),
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test nested map and filter
TEST_F(ComputoTest, NestedMapFilter) {
    // ["map", ["filter", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], ["$", "/x"]]], ["lambda", ["x"], ["+", ["$", "/x"], 10]]]
    json script = json::array({
        "map",
        json::array({
            "filter",
            json{{"array", json::array({1, 2, 3, 4, 5})}},
            json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})}) // All pass (all truthy)
        }),
        json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 10})})
    });
    json expected = json::array({11, 12, 13, 14, 15});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test map with let variables
TEST_F(ComputoTest, MapWithLet) {
    // ["let", [["multiplier", 3]], ["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["+", ["$", "/x"], ["$", "/multiplier"]]]]]
    json script = json::array({
        "let",
        json::array({json::array({"multiplier", 3})}),
        json::array({
            "map",
            json{{"array", json::array({1, 2, 3})}},
            json::array({
                "lambda",
                json::array({"x"}),
                json::array({"+", json::array({"$", "/x"}), json::array({"$", "/multiplier"})})
            })
        })
    });
    json expected = json::array({4, 5, 6});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test map with conditional in lambda
TEST_F(ComputoTest, MapWithConditional) {
    // ["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["if", ["$", "/x"], ["+", ["$", "/x"], 10], 0]]]
    json script = json::array({
        "map",
        json{{"array", json::array({1, 2, 3})}},
        json::array({
            "lambda",
            json::array({"x"}),
            json::array({
                "if",
                json::array({"$", "/x"}),
                json::array({"+", json::array({"$", "/x"}), 10}),
                0
            })
        })
    });
    json expected = json::array({11, 12, 13});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// === Error Tests for Phase 5 ===

// Test map operator errors
TEST_F(ComputoTest, MapOperatorWrongArgCount) {
    json script1 = json::array({"map", json::array({1, 2, 3})});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"map", json::array({1, 2, 3}), json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})}), "extra"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, MapOperatorNonArray) {
    json script = json::array({
        "map",
        "not_an_array",
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test filter operator errors
TEST_F(ComputoTest, FilterOperatorWrongArgCount) {
    json script1 = json::array({"filter", json::array({1, 2, 3})});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, FilterOperatorNonArray) {
    json script = json::array({
        "filter",
        42,
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test lambda errors
TEST_F(ComputoTest, LambdaInvalidFormat) {
    // Wrong number of arguments
    json script1 = json::array({
        "map",
        json::array({1, 2}),
        json::array({"lambda", json::array({"x"})}) // Missing body
    });
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Not starting with "lambda"
    json script2 = json::array({
        "map",
        json::array({1, 2}),
        json::array({"not_lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, LambdaInvalidParameters) {
    // Non-array parameters
    json script1 = json::array({
        "map",
        json::array({1, 2}),
        json::array({"lambda", "not_array", json::array({"$", "/x"})})
    });
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Wrong number of parameters
    json script2 = json::array({
        "map",
        json::array({1, 2}),
        json::array({"lambda", json::array({"x", "y"}), json::array({"$", "/x"})}) // Too many params
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Non-string parameter
    json script3 = json::array({
        "map",
        json::array({1, 2}),
        json::array({"lambda", json::array({123}), json::array({"$", "/x"})})
    });
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// === Phase 6 Tests: Additional Array Utilities and Comparison Operators ===

// Test find operator - finds first matching element
TEST_F(ComputoTest, FindOperatorBasic) {
    // ["find", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 3]]]
    json script = json::array({
        "find",
        json{{"array", json::array({1, 2, 3, 4, 5})}},
        json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 3})})
    });
    json expected = 4; // First element > 3
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test find operator - returns null when no match
TEST_F(ComputoTest, FindOperatorNoMatch) {
    // ["find", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 10]]]
    json script = json::array({
        "find",
        json{{"array", json::array({1, 2, 3})}},
        json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 10})})
    });
    json expected = nullptr; // No element > 10
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test find operator on empty array
TEST_F(ComputoTest, FindOperatorEmpty) {
    json script = json::array({
        "find",
        json{{"array", json::array()}},
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    json expected = nullptr;
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test some operator - returns true when any match
TEST_F(ComputoTest, SomeOperatorTrue) {
    // ["some", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 3]]]
    json script = json::array({
        "some",
        json{{"array", json::array({1, 2, 3, 4, 5})}},
        json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 3})})
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test some operator - returns false when no match
TEST_F(ComputoTest, SomeOperatorFalse) {
    // ["some", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 10]]]
    json script = json::array({
        "some",
        json{{"array", json::array({1, 2, 3})}},
        json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 10})})
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test some operator on empty array
TEST_F(ComputoTest, SomeOperatorEmpty) {
    json script = json::array({
        "some",
        json{{"array", json::array()}},
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test every operator - returns true when all match
TEST_F(ComputoTest, EveryOperatorTrue) {
    // ["every", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 0]]]
    json script = json::array({
        "every",
        json{{"array", json::array({1, 2, 3, 4, 5})}},
        json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 0})})
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test every operator - returns false when not all match
TEST_F(ComputoTest, EveryOperatorFalse) {
    // ["every", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 3]]]
    json script = json::array({
        "every",
        json{{"array", json::array({1, 2, 3, 4, 5})}},
        json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 3})})
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test every operator on empty array (vacuous truth)
TEST_F(ComputoTest, EveryOperatorEmpty) {
    json script = json::array({
        "every",
        json{{"array", json::array()}},
        json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 10})})
    });
    EXPECT_EQ(computo::execute(script, input_data), true); // Vacuous truth
}

// Test flatMap operator - maps and flattens
TEST_F(ComputoTest, FlatMapOperatorBasic) {
    // ["flatMap", {"array": [1, 2, 3]}, ["lambda", ["x"], {"array": [["$", "/x"], ["*", ["$", "/x"], 2]]}]]
    json script = json::array({
        "flatMap",
        json{{"array", json::array({1, 2, 3})}},
        json::array({
            "lambda", 
            json::array({"x"}), 
            json{{"array", json::array({
                json::array({"$", "/x"}),
                json::array({"*", json::array({"$", "/x"}), 2})
            })}}
        })
    });
    json expected = json::array({1, 2, 2, 4, 3, 6}); // [1,2] + [2,4] + [3,6] flattened
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test flatMap operator with non-array results (should include as-is)
TEST_F(ComputoTest, FlatMapOperatorNonArray) {
    // ["flatMap", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]]
    json script = json::array({
        "flatMap",
        json{{"array", json::array({1, 2, 3})}},
        json::array({"lambda", json::array({"x"}), json::array({"*", json::array({"$", "/x"}), 2})})
    });
    json expected = json::array({2, 4, 6}); // Non-array results included as-is
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test flatMap operator on empty array
TEST_F(ComputoTest, FlatMapOperatorEmpty) {
    json script = json::array({
        "flatMap",
        json{{"array", json::array()}},
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test approx operator - epsilon-based floating point equality
TEST_F(ComputoTest, ApproxOperatorTrue) {
    // ["approx", 3.14159, 3.14160, 0.001]
    json script = json::array({"approx", 3.14159, 3.14160, 0.001});
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test approx operator - values too far apart
TEST_F(ComputoTest, ApproxOperatorFalse) {
    // ["approx", 3.14159, 3.14160, 0.000001]
    json script = json::array({"approx", 3.14159, 3.14160, 0.000001});
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test approx operator with integers
TEST_F(ComputoTest, ApproxOperatorIntegers) {
    // ["approx", 5, 5, 0.1]
    json script = json::array({"approx", 5, 5, 0.1});
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test comparison operators
TEST_F(ComputoTest, ComparisonOperators) {
    // Test greater than
    json script_gt = json::array({">", 5, 3});
    EXPECT_EQ(computo::execute(script_gt, input_data), true);
    
    json script_gt_false = json::array({">", 3, 5});
    EXPECT_EQ(computo::execute(script_gt_false, input_data), false);
    
    // Test less than
    json script_lt = json::array({"<", 3, 5});
    EXPECT_EQ(computo::execute(script_lt, input_data), true);
    
    // Test greater than or equal
    json script_gte = json::array({">=", 5, 5});
    EXPECT_EQ(computo::execute(script_gte, input_data), true);
    
    // Test less than or equal
    json script_lte = json::array({"<=", 3, 5});
    EXPECT_EQ(computo::execute(script_lte, input_data), true);
    
    // Test equality
    json script_eq = json::array({"==", 5, 5});
    EXPECT_EQ(computo::execute(script_eq, input_data), true);
    
    // Test inequality
    json script_neq = json::array({"!=", 5, 3});
    EXPECT_EQ(computo::execute(script_neq, input_data), true);
}

// Test math operators
TEST_F(ComputoTest, MathOperators) {
    // Test subtraction
    json script_sub = json::array({"-", 10, 3});
    EXPECT_EQ(computo::execute(script_sub, input_data), 7);
    
    // Test multiplication
    json script_mul = json::array({"*", 4, 5});
    EXPECT_EQ(computo::execute(script_mul, input_data), 20);
    
    // Test division
    json script_div = json::array({"/", 15, 3});
    EXPECT_EQ(computo::execute(script_div, input_data), 5.0);
    
    // Test division with floats
    json script_div_float = json::array({"/", 7, 2});
    EXPECT_EQ(computo::execute(script_div_float, input_data), 3.5);
}

// Test error conditions for new operators
TEST_F(ComputoTest, FindOperatorWrongArgCount) {
    json script = json::array({"find", json{{"array", json::array({1, 2, 3})}}});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, SomeOperatorNonArray) {
    json script = json::array({
        "some",
        "not_an_array",
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, ApproxOperatorWrongArgCount) {
    json script = json::array({"approx", 3.14, 3.15});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, ApproxOperatorNonNumeric) {
    json script = json::array({"approx", "not_a_number", 3.14, 0.01});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, ApproxOperatorNegativeEpsilon) {
    json script = json::array({"approx", 3.14, 3.15, -0.01});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, DivisionByZero) {
    json script = json::array({"/", 5, 0});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test complex integration with new operators
TEST_F(ComputoTest, Phase6Integration) {
    // Complex example: find sum of squares of even numbers > 2
    // ["reduce", 
    //   ["filter", 
    //     ["map", {"array": [1,2,3,4,5,6]}, ["lambda", ["x"], ["*", ["$", "/x"], ["$", "/x"]]]], 
    //     ["lambda", ["sq"], [">=", ["$", "/sq"], 9]]],
    //   ["lambda", ["acc", "item"], ["+", ["$", "/acc"], ["$", "/item"]]], 
    //   0]
    json script = json::array({
        "reduce",
        json::array({
            "filter",
            json::array({
                "map",
                json{{"array", json::array({1, 2, 3, 4, 5, 6})}},
                json::array({"lambda", json::array({"x"}), json::array({"*", json::array({"$", "/x"}), json::array({"$", "/x"})})})
            }),
            json::array({"lambda", json::array({"sq"}), json::array({">=", json::array({"$", "/sq"}), 9})})
        }),
        json::array({"lambda", json::array({"acc", "item"}), json::array({"+", json::array({"$", "/acc"}), json::array({"$", "/item"})})}),
        0
    });
    
    // Squares: [1,4,9,16,25,36], filtered >=9: [9,16,25,36], sum: 86
    json expected = 86;
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test count operator
TEST_F(ComputoTest, CountOperator) {
    // Test basic count
    json script = json::array({"count", json{{"array", json::array({1, 2, 3, 4, 5})}}});
    json expected = 5;
    EXPECT_EQ(computo::execute(script, input_data), expected);
    
    // Test count with empty array
    json script_empty = json::array({"count", json{{"array", json::array()}}});
    json expected_empty = 0;
    EXPECT_EQ(computo::execute(script_empty, input_data), expected_empty);
    
    // Test count with array using let binding
    json script_input = json::array({
        "let", 
        json::array({json::array({"myarray", json{{"array", json::array({10, 20, 30})}}})}),
        json::array({"count", json::array({"$", "/myarray"})})
    });
    json expected_input = 3;
    EXPECT_EQ(computo::execute(script_input, input_data), expected_input);
}

// Test count operator errors
TEST_F(ComputoTest, CountOperatorErrors) {
    // Test count with non-array
    json script_invalid = json::array({"count", "not an array"});
    EXPECT_THROW(computo::execute(script_invalid, input_data), computo::InvalidArgumentException);
    
    // Test count with wrong number of arguments
    json script_no_args = json::array({"count"});
    EXPECT_THROW(computo::execute(script_no_args, input_data), computo::InvalidArgumentException);
    
    json script_too_many = json::array({"count", json{{"array", json::array({1, 2, 3})}}, "extra"});
    EXPECT_THROW(computo::execute(script_too_many, input_data), computo::InvalidArgumentException);
}