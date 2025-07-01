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

// Test addition with one argument - now valid (returns the argument)
TEST_F(ComputoTest, AdditionOneArgument) {
    json script = json::array({"+", 5});
    EXPECT_EQ(computo::execute(script, input_data), 5);
}

// Test addition with multiple arguments - now valid (n-ary)
TEST_F(ComputoTest, AdditionMultipleArguments) {
    json script = json::array({"+", 1, 2, 3, 4});
    EXPECT_EQ(computo::execute(script, input_data), 10);
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

TEST_F(ComputoTest, ObjOperatorComputedKeyNonString) {
    // Computed key that evaluates to non-string
    json script = json::array({
        "let",
        json::array({json::array({"num", 123})}),
        json::array({
            "obj",
            json::array({json::array({"$", "/num"}), "value"})
        })
    });
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
        json::array({"lambda", json::array({"x", "y"}), json::array({"$", "/x"})})
    });
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Wrong number of parameters
    json script2 = json::array({
        "map",
        json::array({1, 2}),
        json::array({"lambda", json::array({"x", "y"}), json::array({"$", "/x"})})
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

// === Tests for New Diff/Patch Features ===

// Test $inputs system variable
TEST_F(ComputoTest, InputsSystemVariable) {
    std::vector<json> inputs = {
        json{{"id", 1}, {"name", "first"}},
        json{{"id", 2}, {"name", "second"}},
        json{{"id", 3}, {"name", "third"}}
    };
    
    json script = json::array({"$inputs"});
    json result = computo::execute(script, inputs);
    
    json expected = json::array({
        json{{"id", 1}, {"name", "first"}},
        json{{"id", 2}, {"name", "second"}},
        json{{"id", 3}, {"name", "third"}}
    });
    
    EXPECT_EQ(result, expected);
}

// Test $inputs with empty inputs
TEST_F(ComputoTest, InputsSystemVariableEmpty) {
    std::vector<json> inputs = {};
    
    json script = json::array({"$inputs"});
    json result = computo::execute(script, inputs);
    
    json expected = json::array();
    EXPECT_EQ(result, expected);
}

// Test $inputs with single input
TEST_F(ComputoTest, InputsSystemVariableSingle) {
    std::vector<json> inputs = {json{{"test", "value"}}};
    
    json script = json::array({"$inputs"});
    json result = computo::execute(script, inputs);
    
    json expected = json::array({json{{"test", "value"}}});
    EXPECT_EQ(result, expected);
}

// Test backward compatibility: $input with multiple inputs
TEST_F(ComputoTest, InputBackwardCompatibilityMultiple) {
    std::vector<json> inputs = {
        json{{"id", 1}, {"name", "first"}},
        json{{"id", 2}, {"name", "second"}}
    };
    
    json script = json::array({"$input"});
    json result = computo::execute(script, inputs);
    
    // Should return the first input
    json expected = json{{"id", 1}, {"name", "first"}};
    EXPECT_EQ(result, expected);
}

// Test backward compatibility: $input with empty inputs
TEST_F(ComputoTest, InputBackwardCompatibilityEmpty) {
    std::vector<json> inputs = {};
    
    json script = json::array({"$input"});
    json result = computo::execute(script, inputs);
    
    // Should return null when no inputs
    EXPECT_EQ(result, json(nullptr));
}

// Test accessing specific inputs with get and $inputs
TEST_F(ComputoTest, InputsWithGetOperator) {
    std::vector<json> inputs = {
        json{{"id", 1}, {"name", "first"}},
        json{{"id", 2}, {"name", "second"}},
        json{{"id", 3}, {"name", "third"}}
    };
    
    // Get the second input: ["get", ["$inputs"], "/1"]
    json script = json::array({
        "get",
        json::array({"$inputs"}),
        "/1"
    });
    
    json result = computo::execute(script, inputs);
    json expected = json{{"id", 2}, {"name", "second"}};
    EXPECT_EQ(result, expected);
}

// Test diff operator basic functionality
TEST_F(ComputoTest, DiffOperatorBasic) {
    json original = json{{"id", 123}, {"status", "active"}};
    json modified = json{{"id", 123}, {"status", "archived"}};
    
    json script = json::array({
        "diff",
        original,
        modified
    });
    
    json result = computo::execute(script, input_data);
    
    // Should contain a replace operation for status
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0]["op"], "replace");
    EXPECT_EQ(result[0]["path"], "/status");
    EXPECT_EQ(result[0]["value"], "archived");
}

// Test diff operator with identical documents
TEST_F(ComputoTest, DiffOperatorIdentical) {
    json document = json{{"id", 123}, {"name", "test"}};
    
    json script = json::array({
        "diff",
        document,
        document
    });
    
    json result = computo::execute(script, input_data);
    
    // Should return empty array for identical documents
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 0);
}

// Test diff operator with complex changes
TEST_F(ComputoTest, DiffOperatorComplex) {
    json original = json{
        {"id", 123},
        {"user", json{{"name", "John"}, {"age", 30}}},
        {"tags", json::array({"tag1", "tag2"})}
    };
    
    json modified = json{
        {"id", 123},
        {"user", json{{"name", "Jane"}, {"age", 31}, {"email", "jane@example.com"}}},
        {"tags", json::array({"tag1", "tag3", "tag4"})}
    };
    
    json script = json::array({
        "diff",
        original,
        modified
    });
    
    json result = computo::execute(script, input_data);
    
    // Should contain multiple operations
    EXPECT_TRUE(result.is_array());
    EXPECT_GT(result.size(), 0);
    
    // Test the basic functionality of diff - we'll test round-trip in a separate test
    // Just verify we get some patch operations that look correct
    bool found_user_change = false;
    bool found_tags_change = false;
    
    for (const auto& op : result) {
        if (op.contains("path")) {
            std::string path = op["path"];
            if (path.find("/user/") != std::string::npos) {
                found_user_change = true;
            }
            if (path.find("/tags") != std::string::npos) {
                found_tags_change = true;
            }
        }
    }
    
    EXPECT_TRUE(found_user_change || found_tags_change); // At least one change should be detected
}

// Test patch operator basic functionality
TEST_F(ComputoTest, PatchOperatorBasic) {
    json document = json{{"id", 123}, {"status", "active"}};
    json patch = json{{"array", json::array({
        json{{"op", "replace"}, {"path", "/status"}, {"value", "archived"}}
    })}};
    
    json script = json::array({
        "patch",
        document,
        patch
    });
    
    json result = computo::execute(script, input_data);
    json expected = json{{"id", 123}, {"status", "archived"}};
    
    EXPECT_EQ(result, expected);
}

// Test patch operator with add operation
TEST_F(ComputoTest, PatchOperatorAdd) {
    json document = json{{"id", 123}};
    json patch = json{{"array", json::array({
        json{{"op", "add"}, {"path", "/name"}, {"value", "John"}}
    })}};
    
    json script = json::array({
        "patch",
        document,
        patch
    });
    
    json result = computo::execute(script, input_data);
    json expected = json{{"id", 123}, {"name", "John"}};
    
    EXPECT_EQ(result, expected);
}

// Test patch operator with remove operation
TEST_F(ComputoTest, PatchOperatorRemove) {
    json document = json{{"id", 123}, {"temp", "to_remove"}};
    json patch = json{{"array", json::array({
        json{{"op", "remove"}, {"path", "/temp"}}
    })}};
    
    json script = json::array({
        "patch",
        document,
        patch
    });
    
    json result = computo::execute(script, input_data);
    json expected = json{{"id", 123}};
    
    EXPECT_EQ(result, expected);
}

// Test patch operator with empty patch
TEST_F(ComputoTest, PatchOperatorEmpty) {
    json document = json{{"id", 123}, {"name", "test"}};
    json patch = json{{"array", json::array()}};
    
    json script = json::array({
        "patch",
        document,
        patch
    });
    
    json result = computo::execute(script, input_data);
    
    // Empty patch should return original document unchanged
    EXPECT_EQ(result, document);
}

// Test patch operator with array operations
TEST_F(ComputoTest, PatchOperatorArray) {
    json document = json{{"items", json::array({1, 2, 3})}};
    json patch = json{{"array", json::array({
        json{{"op", "add"}, {"path", "/items/1"}, {"value", 99}}
    })}};
    
    json script = json::array({
        "patch",
        document,
        patch
    });
    
    json result = computo::execute(script, input_data);
    json expected = json{{"items", json::array({1, 99, 2, 3})}};
    
    EXPECT_EQ(result, expected);
}

// Test combination of $inputs with diff operator
TEST_F(ComputoTest, InputsWithDiffOperator) {
    std::vector<json> inputs = {
        json{{"id", 123}, {"status", "active"}},
        json{{"id", 123}, {"status", "archived"}}
    };
    
    // Generate diff between first and second input
    json script = json::array({
        "diff",
        json::array({"get", json::array({"$inputs"}), "/0"}),
        json::array({"get", json::array({"$inputs"}), "/1"})
    });
    
    json result = computo::execute(script, inputs);
    
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0]["op"], "replace");
    EXPECT_EQ(result[0]["path"], "/status");
    EXPECT_EQ(result[0]["value"], "archived");
}

// Test combination of $inputs with patch operator
TEST_F(ComputoTest, InputsWithPatchOperator) {
    std::vector<json> inputs = {
        json{{"id", 123}, {"status", "active"}}, // document to patch
        json::array({  // patch to apply directly as array
            json{{"op", "replace"}, {"path", "/status"}, {"value", "archived"}}
        })
    };
    
    // Apply patch from second input to first input
    json script = json::array({
        "patch",
        json::array({"get", json::array({"$inputs"}), "/0"}),
        json::array({"get", json::array({"$inputs"}), "/1"})
    });
    
    json result = computo::execute(script, inputs);
    json expected = json{{"id", 123}, {"status", "archived"}};
    
    EXPECT_EQ(result, expected);
}

// Test round-trip: diff then patch
TEST_F(ComputoTest, DiffPatchRoundTrip) {
    json original = json{
        {"id", 123},
        {"user", json{{"name", "John"}, {"age", 30}}},
        {"tags", json::array({"tag1", "tag2"})}
    };
    
    json modified = json{
        {"id", 123},
        {"user", json{{"name", "Jane"}, {"age", 31}, {"active", true}}},
        {"tags", json::array({"tag1", "tag3"})}
    };
    
    // Step 1: Generate diff (test outside of Computo to avoid variable storage issues)
    json diff_script = json::array({"diff", original, modified});
    json patch = computo::execute(diff_script, input_data);
    
    // Step 2: Apply the patch using array object syntax
    json patch_as_array_obj = json{{"array", patch}};
    json patch_script = json::array({"patch", original, patch_as_array_obj});
    json result = computo::execute(patch_script, input_data);
    
    // Result should match modified document
    EXPECT_EQ(result, modified);
}

// === Error Tests for New Features ===

// Test diff operator wrong argument count
TEST_F(ComputoTest, DiffOperatorWrongArgCount) {
    json script1 = json::array({"diff", json{{"a", 1}}});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"diff", json{{"a", 1}}, json{{"a", 2}}, json{{"a", 3}}});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test patch operator wrong argument count
TEST_F(ComputoTest, PatchOperatorWrongArgCount) {
    json script1 = json::array({"patch", json{{"a", 1}}});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"patch", json{{"a", 1}}, json::array(), json{{"extra", true}}});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test patch operator with non-array patch
TEST_F(ComputoTest, PatchOperatorNonArray) {
    json script = json::array({
        "patch",
        json{{"a", 1}},
        json{{"op", "replace"}, {"path", "/a"}, {"value", 2}}  // Object instead of array
    });
    
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test patch operator with malformed patch
TEST_F(ComputoTest, PatchOperatorMalformed) {
    json script = json::array({
        "patch",
        json{{"a", 1}},
        json{{"array", json::array({
            json{{"op", "replace"}, {"path", "/nonexistent"}, {"value", 2}}
        })}}
    });
    
    EXPECT_THROW(computo::execute(script, input_data), computo::PatchFailedException);
}

// Test patch operator with invalid operation
TEST_F(ComputoTest, PatchOperatorInvalidOp) {
    json script = json::array({
        "patch",
        json{{"a", 1}},
        json{{"array", json::array({
            json{{"op", "invalid_op"}, {"path", "/a"}, {"value", 2}}
        })}}
    });
    
    EXPECT_THROW(computo::execute(script, input_data), computo::PatchFailedException);
}

// Test PatchFailedException hierarchy
TEST_F(ComputoTest, PatchFailedExceptionHierarchy) {
    json script = json::array({
        "patch",
        json{{"a", 1}},
        json{{"array", json::array({
            json{{"op", "invalid_op"}, {"path", "/a"}, {"value", 2}}
        })}}
    });
    
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected PatchFailedException to be thrown";
    } catch (const computo::ComputoException& e) {
        // This should catch PatchFailedException since it inherits from ComputoException
        SUCCEED();
    } catch (...) {
        FAIL() << "PatchFailedException should inherit from ComputoException";
    }
}

// Test edge case: diff with null values
TEST_F(ComputoTest, DiffOperatorWithNull) {
    json original = json{{"value", json(nullptr)}};
    json modified = json{{"value", 42}};
    
    json script = json::array({"diff", original, modified});
    json result = computo::execute(script, input_data);
    
    EXPECT_TRUE(result.is_array());
    EXPECT_GT(result.size(), 0);
    
    // Verify round-trip by applying the patch using array object syntax  
    json patch_as_array_obj = json{{"array", result}};
    json patch_script = json::array({"patch", original, patch_as_array_obj});
    json patched = computo::execute(patch_script, input_data);
    EXPECT_EQ(patched, modified);
}

// Test edge case: patch with test operation
TEST_F(ComputoTest, PatchOperatorWithTest) {
    json document = json{{"value", 42}};
    json patch = json{{"array", json::array({
        json{{"op", "test"}, {"path", "/value"}, {"value", 42}},
        json{{"op", "replace"}, {"path", "/value"}, {"value", 100}}
    })}};
    
    json script = json::array({"patch", document, patch});
    json result = computo::execute(script, input_data);
    
    json expected = json{{"value", 100}};
    EXPECT_EQ(result, expected);
}

// Test that a failing 'test' operation in a patch causes the entire patch to fail.
TEST_F(ComputoTest, PatchOperatorWithFailingTest) {
    json document = json{{"value", 42}};
    // This patch should fail because the "test" operation expects the value to be 99, but it is 42.
    json patch = json{{"array", json::array({
        json{{"op", "test"}, {"path", "/value"}, {"value", 99}},
        json{{"op", "replace"}, {"path", "/value"}, {"value", 100}} // This should not be executed.
    })}};

    json script = json::array({"patch", document, patch});

    // The patch operation must throw because the test condition was not met.
    EXPECT_THROW(computo::execute(script, input_data), computo::PatchFailedException);
}

// Test that the diff operator correctly generates an 'add' operation.
TEST_F(ComputoTest, DiffOperatorGeneratesAdd) {
    json original = json{{"id", 123}};
    json modified = json{{"id", 123}, {"name", "new"}};

    json script = json::array({"diff", original, modified});
    json result = computo::execute(script, input_data);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0]["op"], "add");
    EXPECT_EQ(result[0]["path"], "/name");
    EXPECT_EQ(result[0]["value"], "new");
}

// Test that the diff operator correctly generates a 'remove' operation.
TEST_F(ComputoTest, DiffOperatorGeneratesRemove) {
    json original = json{{"id", 123}, {"temp", "delete_me"}};
    json modified = json{{"id", 123}};

    json script = json::array({"diff", original, modified});
    json result = computo::execute(script, input_data);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0]["op"], "remove");
    EXPECT_EQ(result[0]["path"], "/temp");
}

// Test that accessing an out-of-bounds index on the $inputs array throws an exception.
TEST_F(ComputoTest, InputsWithGetOperatorOutOfBounds) {
    std::vector<json> inputs = {
        json{{"id", 1}},
        json{{"id", 2}}
    };

    // Attempt to access index 2, which is out of bounds for an array of size 2.
    json script = json::array({
        "get",
        json::array({"$inputs"}),
        "/2"
    });

    // The 'get' operator should throw when the JSON Pointer path is invalid for the given document.
    EXPECT_THROW(computo::execute(script, inputs), computo::InvalidArgumentException);
}

// Test a more complex round-trip scenario with multiple operations.
TEST_F(ComputoTest, DiffPatchRoundTripComplex) {
    json original = json{{"a", 1}, {"b", json::array({10, 20})}, {"c", "original"}};
    json modified = json{{"b", json::array({10, 25, 30})}, {"c", "modified"}, {"d", true}}; // a is removed, b is changed, d is added

    // Step 1: Generate the diff.
    json diff_script = json::array({"diff", original, modified});
    json patch = computo::execute(diff_script, input_data);

    // Verify the patch has multiple operations (add, remove, replace)
    ASSERT_GE(patch.size(), 3);

    // Step 2: Apply the generated patch to the original document.
    json patch_as_array_obj = json{{"array", patch}};
    json patch_script = json::array({"patch", original, patch_as_array_obj});
    json result = computo::execute(patch_script, input_data);

    // The final result must be identical to the modified document.
    EXPECT_EQ(result, modified);
}

// Test car operator - basic functionality
TEST_F(ComputoTest, CarOperatorBasic) {
    // ["car", {"array": [1, 2, 3]}] -> 1
    json script = json::array({
        "car",
        json{{"array", json::array({1, 2, 3})}}
    });
    EXPECT_EQ(computo::execute(script, input_data), 1);
}

// Test car operator - single element array
TEST_F(ComputoTest, CarOperatorSingleElement) {
    // ["car", {"array": ["hello"]}] -> "hello"
    json script = json::array({
        "car",
        json{{"array", json::array({"hello"})}}
    });
    EXPECT_EQ(computo::execute(script, input_data), "hello");
}

// Test car operator - nested data
TEST_F(ComputoTest, CarOperatorNested) {
    // ["car", {"array": [{"name": "Alice"}, {"name": "Bob"}]}] -> {"name": "Alice"}
    json script = json::array({
        "car",
        json{{"array", json::array({json{{"name", "Alice"}}, json{{"name", "Bob"}}})}}
    });
    json expected = json{{"name", "Alice"}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test car operator - empty array (should throw)
TEST_F(ComputoTest, CarOperatorEmptyArray) {
    json script = json::array({
        "car",
        json{{"array", json::array()}}
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test car operator - non-array input (should throw)
TEST_F(ComputoTest, CarOperatorNonArray) {
    json script = json::array({
        "car",
        "not an array"
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test car operator - wrong argument count (should throw)
TEST_F(ComputoTest, CarOperatorWrongArgCount) {
    json script = json::array({
        "car",
        json{{"array", json::array({1, 2, 3})}},
        json{{"array", json::array({4, 5, 6})}}  // Extra argument
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test car operator - no arguments (should throw)
TEST_F(ComputoTest, CarOperatorNoArgs) {
    json script = json::array({"car"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test cdr operator - basic functionality
TEST_F(ComputoTest, CdrOperatorBasic) {
    // ["cdr", {"array": [1, 2, 3]}] -> [2, 3]
    json script = json::array({
        "cdr",
        json{{"array", json::array({1, 2, 3})}}
    });
    json expected = json::array({2, 3});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test cdr operator - single element array
TEST_F(ComputoTest, CdrOperatorSingleElement) {
    // ["cdr", {"array": ["hello"]}] -> []
    json script = json::array({
        "cdr",
        json{{"array", json::array({"hello"})}}
    });
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test cdr operator - two element array
TEST_F(ComputoTest, CdrOperatorTwoElements) {
    // ["cdr", {"array": [1, 2]}] -> [2]
    json script = json::array({
        "cdr",
        json{{"array", json::array({1, 2})}}
    });
    json expected = json::array({2});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test cdr operator - nested data
TEST_F(ComputoTest, CdrOperatorNested) {
    // ["cdr", {"array": [{"name": "Alice"}, {"name": "Bob"}, {"name": "Carol"}]}] -> [{"name": "Bob"}, {"name": "Carol"}]
    json script = json::array({
        "cdr",
        json{{"array", json::array({
            json{{"name", "Alice"}}, 
            json{{"name", "Bob"}}, 
            json{{"name", "Carol"}}
        })}}
    });
    json expected = json::array({json{{"name", "Bob"}}, json{{"name", "Carol"}}});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test cdr operator - empty array (should return empty array)
TEST_F(ComputoTest, CdrOperatorEmptyArray) {
    json script = json::array({
        "cdr",
        json{{"array", json::array()}}
    });
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test cdr operator - non-array input (should throw)
TEST_F(ComputoTest, CdrOperatorNonArray) {
    json script = json::array({
        "cdr",
        "not an array"
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test cdr operator - wrong argument count (should throw)
TEST_F(ComputoTest, CdrOperatorWrongArgCount) {
    json script = json::array({
        "cdr",
        json{{"array", json::array({1, 2, 3})}},
        json{{"array", json::array({4, 5, 6})}}  // Extra argument
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test cdr operator - no arguments (should throw)
TEST_F(ComputoTest, CdrOperatorNoArgs) {
    json script = json::array({"cdr"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test car and cdr composition
TEST_F(ComputoTest, CarCdrComposition) {
    // ["car", ["cdr", {"array": [1, 2, 3]}]] -> car([2, 3]) -> 2
    json script = json::array({
        "car",
        json::array({
            "cdr", 
            json{{"array", json::array({1, 2, 3})}}
        })
    });
    EXPECT_EQ(computo::execute(script, input_data), 2);
}

// Test cdr of cdr
TEST_F(ComputoTest, CdrCdrComposition) {
    // ["cdr", ["cdr", {"array": [1, 2, 3, 4]}]] -> cdr([2, 3, 4]) -> [3, 4]
    json script = json::array({
        "cdr",
        json::array({
            "cdr", 
            json{{"array", json::array({1, 2, 3, 4})}}
        })
    });
    json expected = json::array({3, 4});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test car and cdr with $inputs (for multiple inputs use case)
TEST_F(ComputoTest, CarCdrWithInputs) {
    // Create multiple inputs scenario
    std::vector<json> multiple_inputs = {
        json{{"id", "conv_001"}},  // Initial conversation
        json::array({json{{"op", "add"}, {"path", "/messages/0"}, {"value", "Hello"}}}),  // First patch
        json::array({json{{"op", "add"}, {"path", "/messages/1"}, {"value", "Hi there"}}})  // Second patch
    };
    
    // ["car", ["$inputs"]] -> first input (initial conversation)
    json script = json::array({
        "car",
        json::array({"$inputs"})
    });
    json result = computo::execute(script, multiple_inputs);
    json expected_result = json{{"id", "conv_001"}};
    EXPECT_EQ(result, expected_result);
    
    // ["cdr", ["$inputs"]] -> all but first input (patches)
    json script2 = json::array({
        "cdr", 
        json::array({"$inputs"})
    });
    json result2 = computo::execute(script2, multiple_inputs);
    json expected = json::array({
        json::array({json{{"op", "add"}, {"path", "/messages/0"}, {"value", "Hello"}}}),
        json::array({json{{"op", "add"}, {"path", "/messages/1"}, {"value", "Hi there"}}})
    });
    EXPECT_EQ(result2, expected);
}

// === Tests for Logical Operators && and || ===

// Test && operator with all true values
TEST_F(ComputoTest, AndOperatorAllTrue) {
    // ["&&", true, 1, "hello", {"array": [1]}] -> true
    json script = json::array({
        "&&", 
        true, 
        1, 
        "hello", 
        json{{"array", json::array({1})}}
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test && operator with false value (short-circuit)
TEST_F(ComputoTest, AndOperatorWithFalse) {
    // ["&&", true, false, "hello"] -> false (stops at false)
    json script = json::array({
        "&&", 
        true, 
        false, 
        "hello"
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with zero (falsy)
TEST_F(ComputoTest, AndOperatorWithZero) {
    // ["&&", true, 0, "hello"] -> false (stops at 0)
    json script = json::array({
        "&&", 
        true, 
        0, 
        "hello"
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with empty string (falsy)
TEST_F(ComputoTest, AndOperatorWithEmptyString) {
    // ["&&", true, "", "hello"] -> false (stops at "")
    json script = json::array({
        "&&", 
        true, 
        "", 
        "hello"
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with null (falsy)
TEST_F(ComputoTest, AndOperatorWithNull) {
    // ["&&", true, null, "hello"] -> false (stops at null)
    json script = json::array({
        "&&", 
        true, 
        json(nullptr), 
        "hello"
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with empty array (falsy)
TEST_F(ComputoTest, AndOperatorWithEmptyArray) {
    // ["&&", true, {"array": []}, "hello"] -> false (stops at empty array)
    json script = json::array({
        "&&", 
        true, 
        json{{"array", json::array()}}, 
        "hello"
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with empty object (falsy)
TEST_F(ComputoTest, AndOperatorWithEmptyObject) {
    // ["&&", true, {}, "hello"] -> false (stops at empty object)
    json script = json::array({
        "&&", 
        true, 
        json::object(), 
        "hello"
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with single argument
TEST_F(ComputoTest, AndOperatorSingleArg) {
    // ["&&", true] -> true
    json script = json::array({"&&", true});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // ["&&", false] -> false
    json script2 = json::array({"&&", false});
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

// Test && operator with expressions
TEST_F(ComputoTest, AndOperatorWithExpressions) {
    // ["&&", [">", 5, 3], ["<", 2, 4], ["==", "hello", "hello"]] -> true
    json script = json::array({
        "&&",
        json::array({">", 5, 3}),
        json::array({"<", 2, 4}),
        json::array({"==", "hello", "hello"})
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test && operator short-circuit with expensive expressions
TEST_F(ComputoTest, AndOperatorShortCircuit) {
    // ["&&", false, ["/", 1, 0]] -> false (division by zero not evaluated)
    json script = json::array({
        "&&",
        false,
        json::array({"/", 1, 0})  // This would throw if evaluated
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test || operator with all false values
TEST_F(ComputoTest, OrOperatorAllFalse) {
    // ["||", false, 0, "", null, {"array": []}, {}] -> false
    json script = json::array({
        "||", 
        false, 
        0, 
        "", 
        json(nullptr),
        json{{"array", json::array()}},
        json::object()
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test || operator with true value (short-circuit)
TEST_F(ComputoTest, OrOperatorWithTrue) {
    // ["||", false, true, "hello"] -> true (stops at true)
    json script = json::array({
        "||", 
        false, 
        true, 
        "hello"
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test || operator with truthy value
TEST_F(ComputoTest, OrOperatorWithTruthy) {
    // ["||", false, "hello", true] -> true (stops at "hello")
    json script = json::array({
        "||", 
        false, 
        "hello", 
        true
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test || operator with single argument
TEST_F(ComputoTest, OrOperatorSingleArg) {
    // ["||", true] -> true
    json script = json::array({"||", true});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // ["||", false] -> false
    json script2 = json::array({"||", false});
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

// Test || operator with expressions
TEST_F(ComputoTest, OrOperatorWithExpressions) {
    // ["||", [">", 3, 5], ["<", 4, 2], ["==", "hello", "world"]] -> false
    json script = json::array({
        "||",
        json::array({">", 3, 5}),  // false
        json::array({"<", 4, 2}),  // false
        json::array({"==", "hello", "world"})  // false
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
    
    // ["||", [">", 3, 5], ["<", 2, 4], ["==", "hello", "world"]] -> true (stops at second)
    json script2 = json::array({
        "||",
        json::array({">", 3, 5}),  // false
        json::array({"<", 2, 4}),  // true
        json::array({"==", "hello", "world"})  // not evaluated
    });
    EXPECT_EQ(computo::execute(script2, input_data), true);
}

// Test || operator short-circuit with expensive expressions
TEST_F(ComputoTest, OrOperatorShortCircuit) {
    // ["||", true, ["/", 1, 0]] -> true (division by zero not evaluated)
    json script = json::array({
        "||",
        true,
        json::array({"/", 1, 0})  // This would throw if evaluated
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test nested && and || operators
TEST_F(ComputoTest, NestedLogicalOperators) {
    // ["&&", ["||", false, true], ["||", false, false, true]] -> true
    json script = json::array({
        "&&",
        json::array({"||", false, true}),  // true
        json::array({"||", false, false, true})  // true
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // ["||", ["&&", false, true], ["&&", true, true]] -> true
    json script2 = json::array({
        "||",
        json::array({"&&", false, true}),  // false
        json::array({"&&", true, true})    // true
    });
    EXPECT_EQ(computo::execute(script2, input_data), true);
}

// Test && and || in if conditions
TEST_F(ComputoTest, LogicalOperatorsInIfConditions) {
    // ["if", ["&&", true, ["==", "hello", "hello"]], "success", "failure"]
    json script = json::array({
        "if",
        json::array({"&&", true, json::array({"==", "hello", "hello"})}),
        "success",
        "failure"
    });
    EXPECT_EQ(computo::execute(script, input_data), "success");
    
    // ["if", ["||", false, ["==", "world", "world"]], "success", "failure"]
    json script2 = json::array({
        "if",
        json::array({"||", false, json::array({"==", "world", "world"})}),
        "success",
        "failure"
    });
    EXPECT_EQ(computo::execute(script2, input_data), "success");
}

// Test && and || with let variables
TEST_F(ComputoTest, LogicalOperatorsWithLet) {
    // ["let", [["x", true], ["y", false]], ["&&", ["$", "/x"], ["$", "/y"]]]
    json script = json::array({
        "let",
        json::array({
            json::array({"x", true}),
            json::array({"y", false})
        }),
        json::array({"&&", json::array({"$", "/x"}), json::array({"$", "/y"})})
    });
    EXPECT_EQ(computo::execute(script, input_data), false);
    
    // ["let", [["a", false], ["b", true]], ["||", ["$", "/a"], ["$", "/b"]]]
    json script2 = json::array({
        "let",
        json::array({
            json::array({"a", false}),
            json::array({"b", true})
        }),
        json::array({"||", json::array({"$", "/a"}), json::array({"$", "/b"})})
    });
    EXPECT_EQ(computo::execute(script2, input_data), true);
}

// Test complex logical expression
TEST_F(ComputoTest, ComplexLogicalExpression) {
    // ["&&", ["||", false, true], ["||", ["==", 1, 1], ["!=", 2, 2]], [">", 10, 5]]
    json script = json::array({
        "&&",
        json::array({"||", false, true}),  // true
        json::array({"||", json::array({"==", 1, 1}), json::array({"!=", 2, 2})}),  // true || false = true
        json::array({">", 10, 5})  // true
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// === Error Tests for Logical Operators ===

// Test && operator with no arguments
TEST_F(ComputoTest, AndOperatorNoArgs) {
    json script = json::array({"&&"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test || operator with no arguments
TEST_F(ComputoTest, OrOperatorNoArgs) {
    json script = json::array({"||"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// === Tests for Additional Operators ===

// Test cons operator
TEST_F(ComputoTest, ConsOperator) {
    // ["cons", 1, {"array": [2, 3]}] should return [1, 2, 3]
    json script = json::array({
        "cons", 
        1, 
        json{{"array", json::array({2, 3})}}
    });
    json expected = json::array({1, 2, 3});
    EXPECT_EQ(computo::execute(script, input_data), expected);
    
    // Test with empty array
    json script2 = json::array({
        "cons", 
        "first", 
        json{{"array", json::array()}}
    });
    json expected2 = json::array({"first"});
    EXPECT_EQ(computo::execute(script2, input_data), expected2);
}

// Test cons operator error cases
TEST_F(ComputoTest, ConsOperatorErrors) {
    // Wrong number of arguments
    json script1 = json::array({"cons", 1});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Second argument not an array
    json script2 = json::array({"cons", 1, "not_array"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test append operator
TEST_F(ComputoTest, AppendOperator) {
    // ["append", {"array": [1, 2]}, {"array": [3, 4]}] should return [1, 2, 3, 4]
    json script = json::array({
        "append",
        json{{"array", json::array({1, 2})}},
        json{{"array", json::array({3, 4})}}
    });
    json expected = json::array({1, 2, 3, 4});
    EXPECT_EQ(computo::execute(script, input_data), expected);
    
    // Test with multiple arrays
    json script2 = json::array({
        "append",
        json{{"array", json::array({1})}},
        json{{"array", json::array({2, 3})}},
        json{{"array", json::array({4, 5, 6})}}
    });
    json expected2 = json::array({1, 2, 3, 4, 5, 6});
    EXPECT_EQ(computo::execute(script2, input_data), expected2);
    
    // Test with empty arrays
    json script3 = json::array({
        "append",
        json{{"array", json::array()}},
        json{{"array", json::array({1, 2})}},
        json{{"array", json::array()}}
    });
    json expected3 = json::array({1, 2});
    EXPECT_EQ(computo::execute(script3, input_data), expected3);
}

// Test append operator error cases
TEST_F(ComputoTest, AppendOperatorErrors) {
    // No arguments
    json script1 = json::array({"append"});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Non-array argument
    json script2 = json::array({"append", json{{"array", json::array({1, 2})}}, "not_array"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test chunk operator
TEST_F(ComputoTest, ChunkOperator) {
    // ["chunk", {"array": [1, 2, 3, 4, 5]}, 2] should return [[1, 2], [3, 4], [5]]
    json script = json::array({
        "chunk",
        json{{"array", json::array({1, 2, 3, 4, 5})}},
        2
    });
    json expected = json::array({
        json::array({1, 2}),
        json::array({3, 4}),
        json::array({5})
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
    
    // Test with exact division
    json script2 = json::array({
        "chunk",
        json{{"array", json::array({1, 2, 3, 4})}},
        2
    });
    json expected2 = json::array({
        json::array({1, 2}),
        json::array({3, 4})
    });
    EXPECT_EQ(computo::execute(script2, input_data), expected2);
    
    // Test with chunk size 1
    json script3 = json::array({
        "chunk",
        json{{"array", json::array({"a", "b"})}},
        1
    });
    json expected3 = json::array({
        json::array({"a"}),
        json::array({"b"})
    });
    EXPECT_EQ(computo::execute(script3, input_data), expected3);
    
    // Test with empty array
    json script4 = json::array({
        "chunk",
        json{{"array", json::array()}},
        3
    });
    json expected4 = json::array();
    EXPECT_EQ(computo::execute(script4, input_data), expected4);
}

// Test chunk operator error cases
TEST_F(ComputoTest, ChunkOperatorErrors) {
    // Wrong number of arguments
    json script1 = json::array({"chunk", json{{"array", json::array({1, 2})}}});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // First argument not an array
    json script2 = json::array({"chunk", "not_array", 2});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Second argument not a positive integer
    json script3 = json::array({"chunk", json{{"array", json::array({1, 2})}}, 0});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
    
    json script4 = json::array({"chunk", json{{"array", json::array({1, 2})}}, -1});
    EXPECT_THROW(computo::execute(script4, input_data), computo::InvalidArgumentException);
    
    json script5 = json::array({"chunk", json{{"array", json::array({1, 2})}}, 2.5});
    EXPECT_THROW(computo::execute(script5, input_data), computo::InvalidArgumentException);
}

// Test partition operator
TEST_F(ComputoTest, PartitionOperator) {
    // ["partition", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 3]]]
    // Should return [[4, 5], [1, 2, 3]]
    json script = json::array({
        "partition",
        json{{"array", json::array({1, 2, 3, 4, 5})}},
        json::array({
            "lambda",
            json::array({"x"}),
            json::array({">", json::array({"$", "/x"}), 3})
        })
    });
    json expected = json::array({
        json::array({4, 5}),  // truthy items
        json::array({1, 2, 3})  // falsy items
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
    
    // Test with all items truthy
    json script2 = json::array({
        "partition",
        json{{"array", json::array({1, 2, 3})}},
        json::array({
            "lambda",
            json::array({"x"}),
            true
        })
    });
    json expected2 = json::array({
        json::array({1, 2, 3}),  // all truthy
        json::array()  // none falsy
    });
    EXPECT_EQ(computo::execute(script2, input_data), expected2);
    
    // Test with all items falsy
    json script3 = json::array({
        "partition",
        json{{"array", json::array({1, 2, 3})}},
        json::array({
            "lambda",
            json::array({"x"}),
            false
        })
    });
    json expected3 = json::array({
        json::array(),  // none truthy
        json::array({1, 2, 3})  // all falsy
    });
    EXPECT_EQ(computo::execute(script3, input_data), expected3);
    
    // Test with empty array
    json script4 = json::array({
        "partition",
        json{{"array", json::array()}},
        json::array({
            "lambda",
            json::array({"x"}),
            true
        })
    });
    json expected4 = json::array({
        json::array(),  // empty truthy
        json::array()   // empty falsy
    });
    EXPECT_EQ(computo::execute(script4, input_data), expected4);
}

// Test partition operator error cases
TEST_F(ComputoTest, PartitionOperatorErrors) {
    // Wrong number of arguments
    json script1 = json::array({"partition", json{{"array", json::array({1, 2})}}});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // First argument not an array
    json script2 = json::array({
        "partition", 
        "not_array", 
        json::array({"lambda", json::array({"x"}), true})
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// === Tests for Lambda Variable Resolution ===

// Test basic lambda variable resolution with map
TEST_F(ComputoTest, LambdaVariableResolutionMap) {
    // ["let", [["double", ["lambda", ["x"], ["*", ["$", "/x"], 2]]]], 
    //  ["map", {"array": [1, 2, 3]}, ["$", "/double"]]]
    json script = json::array({
        "let",
        json::array({json::array({"double", json::array({"lambda", json::array({"x"}), json::array({"*", json::array({"$", "/x"}), 2})})})}),
        json::array({
            "map",
            json{{"array", json::array({1, 2, 3})}},
            json::array({"$", "/double"})
        })
    });
    json expected = json::array({2, 4, 6});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test lambda variable resolution with filter
TEST_F(ComputoTest, LambdaVariableResolutionFilter) {
    // ["let", [["gt_two", ["lambda", ["x"], [">", ["$", "/x"], 2]]]], 
    //  ["filter", {"array": [1, 2, 3, 4, 5]}, ["$", "/gt_two"]]]
    json script = json::array({
        "let",
        json::array({json::array({"gt_two", json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 2})})})}),
        json::array({
            "filter",
            json{{"array", json::array({1, 2, 3, 4, 5})}},
            json::array({"$", "/gt_two"})
        })
    });
    json expected = json::array({3, 4, 5});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test lambda variable resolution with reduce
TEST_F(ComputoTest, LambdaVariableResolutionReduce) {
    // ["let", [["add", ["lambda", ["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]]]], 
    //  ["reduce", {"array": [1, 2, 3, 4]}, ["$", "/add"], 0]]
    json script = json::array({
        "let",
        json::array({json::array({"add", json::array({"lambda", json::array({"acc", "x"}), json::array({"+", json::array({"$", "/acc"}), json::array({"$", "/x"})})})})}),
        json::array({
            "reduce",
            json{{"array", json::array({1, 2, 3, 4})}},
            json::array({"$", "/add"}),
            0
        })
    });
    json expected = 10;
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test lambda variable resolution with partition
TEST_F(ComputoTest, LambdaVariableResolutionPartition) {
    // ["let", [["is_even", ["lambda", ["x"], ["==", ["%", ["$", "/x"], 2], 0]]]], 
    //  ["partition", {"array": [1, 2, 3, 4, 5]}, ["$", "/is_even"]]]
    json script = json::array({
        "let",
        json::array({json::array({"is_even", json::array({"lambda", json::array({"x"}), json::array({"==", json::array({"%", json::array({"$", "/x"}), 2}), 0})})})}),
        json::array({
            "partition",
            json{{"array", json::array({1, 2, 3, 4, 5})}},
            json::array({"$", "/is_even"})
        })
    });
    // Note: We don't have % operator, so let's use a simpler test
    json script_simple = json::array({
        "let",
        json::array({json::array({"gt_three", json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 3})})})}),
        json::array({
            "partition",
            json{{"array", json::array({1, 2, 3, 4, 5})}},
            json::array({"$", "/gt_three"})
        })
    });
    json expected = json::array({
        json::array({4, 5}),    // truthy items
        json::array({1, 2, 3})  // falsy items
    });
    EXPECT_EQ(computo::execute(script_simple, input_data), expected);
}

// Test multiple lambda variables in same scope
TEST_F(ComputoTest, MultipleLambdaVariables) {
    // ["let", [["double", ["lambda", ["x"], ["*", ["$", "/x"], 2]]], 
    //          ["increment", ["lambda", ["x"], ["+", ["$", "/x"], 1]]]], 
    //  ["map", ["map", {"array": [1, 2, 3]}, ["$", "/double"]], ["$", "/increment"]]]
    json script = json::array({
        "let",
        json::array({
            json::array({"double", json::array({"lambda", json::array({"x"}), json::array({"*", json::array({"$", "/x"}), 2})})}),
            json::array({"increment", json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})})
        }),
        json::array({
            "map",
            json::array({
                "map",
                json{{"array", json::array({1, 2, 3})}},
                json::array({"$", "/double"})
            }),
            json::array({"$", "/increment"})
        })
    });
    json expected = json::array({3, 5, 7});  // [1,2,3] -> [2,4,6] -> [3,5,7]
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test lambda variable in nested let scopes
TEST_F(ComputoTest, LambdaVariableNestedScopes) {
    // ["let", [["outer_lambda", ["lambda", ["x"], ["*", ["$", "/x"], 2]]]], 
    //  ["let", [["inner_lambda", ["lambda", ["x"], ["+", ["$", "/x"], 1]]]], 
    //   ["map", {"array": [1, 2, 3]}, ["$", "/outer_lambda"]]]]
    json script = json::array({
        "let",
        json::array({json::array({"outer_lambda", json::array({"lambda", json::array({"x"}), json::array({"*", json::array({"$", "/x"}), 2})})})}),
        json::array({
            "let",
            json::array({json::array({"inner_lambda", json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})})}),
            json::array({
                "map",
                json{{"array", json::array({1, 2, 3})}},
                json::array({"$", "/outer_lambda"})  // Access outer scope lambda
            })
        })
    });
    json expected = json::array({2, 4, 6});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test lambda variable resolution error cases
TEST_F(ComputoTest, LambdaVariableResolutionErrors) {
    // Variable not found
    json script1 = json::array({
        "map",
        json{{"array", json::array({1, 2, 3})}},
        json::array({"$", "/nonexistent"})
    });
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Variable contains non-lambda
    json script2 = json::array({
        "let",
        json::array({json::array({"not_lambda", "just_a_string"})}),
        json::array({
            "map",
            json{{"array", json::array({1, 2, 3})}},
            json::array({"$", "/not_lambda"})
        })
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Invalid variable reference format
    json script3 = json::array({
        "let",
        json::array({json::array({"valid_lambda", json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})})}),
        json::array({
            "map",
            json{{"array", json::array({1, 2, 3})}},
            json::array({"$", 42})  // Non-string path
        })
    });
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// === Tests for Enhanced Error Reporting (Path Tracking) ===

// Test basic path tracking in variable lookup
TEST_F(ComputoTest, ErrorPathTrackingBasic) {
    json script = json::array({
        "$", "/nonexistent"
    });
    
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg = e.what();
        EXPECT_NE(error_msg.find("at /$"), std::string::npos) << "Error: " << error_msg;
    }
}

// Test path tracking in nested let expressions
TEST_F(ComputoTest, ErrorPathTrackingNestedLet) {
    json script = json::array({
        "let",
        json::array({json::array({"x", 5})}),
        json::array({
            "obj",
            json::array({"result", json::array({"$", "/missing"})})
        })
    });
    
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg = e.what();
        EXPECT_NE(error_msg.find("at /let/body/obj/$"), std::string::npos) << "Error: " << error_msg;
    }
}

// Test path tracking in map lambda execution
TEST_F(ComputoTest, ErrorPathTrackingMapLambda) {
    json script = json::array({
        "map",
        json{{"array", json::array({1, 2, 3})}},
        json::array({
            "lambda",
            json::array({"x"}),
            json::array({"$", "/undefined"})
        })
    });
    
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg = e.what();
        // Should show error in lambda execution for first element
        EXPECT_NE(error_msg.find("at /map/lambda[0]/$"), std::string::npos) << "Error: " << error_msg;
    }
}

// Test path tracking in complex nested structure
TEST_F(ComputoTest, ErrorPathTrackingComplexNesting) {
    json script = json::array({
        "let",
        json::array({
            json::array({"data", json{{"array", json::array({1, 2, 3})}}})
        }),
        json::array({
            "if",
            true,
            json::array({
                "map",
                json::array({"$", "/data"}),
                json::array({
                    "lambda",
                    json::array({"x"}),
                    json::array({"*", json::array({"$", "/missing"}), 2})
                })
            }),
            json::array({"$", "/fallback"})
        })
    });
    
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg = e.what();
        // Should show path through let->if(then)->map->lambda->multiply->$
        EXPECT_NE(error_msg.find("at /let/body/if/then/map/lambda[0]/*/$"), std::string::npos) << "Error: " << error_msg;
    }
}

// Test path tracking with array syntax error
TEST_F(ComputoTest, ErrorPathTrackingArraySyntax) {
    json script = json::array({
        "let",
        json::array({json::array({"x", 5})}),
        json::array({
            "obj",
            json::array({"bad_array", json::array({42, "not_operator"})})  // Invalid array syntax
        })
    });
    
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg = e.what();
        EXPECT_NE(error_msg.find("at /let/body/obj"), std::string::npos) << "Error: " << error_msg;
        EXPECT_NE(error_msg.find("Array must start with operator name"), std::string::npos) << "Error: " << error_msg;
    }
}

// Test path tracking in if conditions
TEST_F(ComputoTest, ErrorPathTrackingIfCondition) {
    json script = json::array({
        "if",
        json::array({"$", "/missing_condition"}),
        "then_value",
        "else_value"
    });
    
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg = e.what();
        EXPECT_NE(error_msg.find("at /if/condition/$"), std::string::npos) << "Error: " << error_msg;
    }
}

// Test path tracking in array object evaluation
TEST_F(ComputoTest, ErrorPathTrackingArrayObject) {
    json script = json{
        {"array", json::array({
            1,
            json::array({"$", "/missing"}),
            3
        })}
    };
    
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg = e.what();
        EXPECT_NE(error_msg.find("at /array[1]/$"), std::string::npos) << "Error: " << error_msg;
    }
}

// Test path tracking with lambda variable resolution
TEST_F(ComputoTest, ErrorPathTrackingLambdaVariable) {
    json script = json::array({
        "let",
        json::array({
            json::array({"process", json::array({"lambda", json::array({"x"}), json::array({"$", "/missing"})})})
        }),
        json::array({
            "map",
            json{{"array", json::array({1, 2})}},
            json::array({"$", "/process"})
        })
    });
    
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg = e.what();
        // Error occurs in lambda variable execution
        EXPECT_NE(error_msg.find("at /let/body/map/lambda[0]/$"), std::string::npos) << "Error: " << error_msg;
    }
}

// === Multi-Parameter Lambda Tests ===

// Test basic two-parameter lambda with reduce (backward compatibility)
TEST_F(ComputoTest, MultiParamLambdaReduce) {
    json input = json::array({1, 2, 3, 4, 5});
    json script = json::array({
        "reduce",
        json::array({"$input"}),
        json::array({"lambda", json::array({"acc", "item"}), json::array({"+", json::array({"$", "/acc"}), json::array({"$", "/item"})})}),
        0
    });
    EXPECT_EQ(computo::execute(script, input), 15);
}

// Test multi-parameter lambda with zipWith (testing three-param indirectly)
TEST_F(ComputoTest, MultiParamLambdaThreeParamsViaCustom) {
    // We can't directly test 3+ param lambdas without exposing internal functions,
    // but we can test that the multi-parameter system works via zipWith and complex combinations
    json input = json::array({1, 2, 3});
    json script = json::array({
        "let",
        json::array({json::array({
            "combiner",
            json::array({"lambda", json::array({"a", "b"}), json::array({"+", json::array({"*", json::array({"$", "/a"}), 10}), json::array({"$", "/b"})})})
        })}),
        json::array({
            "zipWith",
            json::array({"$input"}),
            json{{"array", json::array({100, 200, 300})}},
            json::array({"$", "/combiner"})
        })
    });
    json expected = json::array({110, 220, 330}); // (1*10)+100, (2*10)+200, (3*10)+300
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test parameter count validation through zipWith
TEST_F(ComputoTest, MultiParamLambdaParameterCountMismatchViaZipWith) {
    json input = json::array({1, 2, 3});
    
    // zipWith provides 2 parameters but lambda only accepts 1
    json script = json::array({
        "zipWith",
        json::array({"$input"}),
        json::array({10, 20, 30}),
        json::array({"lambda", json::array({"a"}), json::array({"$", "/a"})}) // Only 1 param but zipWith provides 2
    });
    
    EXPECT_THROW(computo::execute(script, input), computo::InvalidArgumentException);
}

// Test empty parameter list error through reduce
TEST_F(ComputoTest, MultiParamLambdaEmptyParams) {
    json input = json::array({1, 2, 3});
    
    json script = json::array({
        "reduce",
        json::array({"$input"}),
        json::array({"lambda", json::array(), json::array({"$", "/a"})}), // Empty parameter list
        0
    });
    
    EXPECT_THROW(computo::execute(script, input), computo::InvalidArgumentException);
}

// Test non-string parameter names through reduce
TEST_F(ComputoTest, MultiParamLambdaNonStringParam) {
    json input = json::array({1, 2, 3});
    
    json script = json::array({
        "reduce",
        json::array({"$input"}),
        json::array({"lambda", json::array({123, "item"}), json::array({"+", json::array({"$", "/acc"}), json::array({"$", "/item"})})}), // 123 instead of string
        0
    });
    
    EXPECT_THROW(computo::execute(script, input), computo::InvalidArgumentException);
}

// === zipWith Operator Tests ===

// Test basic zipWith functionality
TEST_F(ComputoTest, ZipWithOperatorBasic) {
    json input = json::object({
        {"array1", json::array({1, 2, 3, 4, 5})},
        {"array2", json::array({10, 20, 30, 40, 50})}
    });
    json script = json::array({
        "zipWith",
        json::array({"get", json::array({"$input"}), "/array1"}),
        json::array({"get", json::array({"$input"}), "/array2"}),
        json::array({"lambda", json::array({"a", "b"}), json::array({"+", json::array({"$", "/a"}), json::array({"$", "/b"})})})
    });
    json expected = json::array({11, 22, 33, 44, 55});
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test zipWith with different sized arrays (takes minimum)
TEST_F(ComputoTest, ZipWithOperatorDifferentSizes) {
    json input = json::object({
        {"array1", json::array({1, 2, 3})},
        {"array2", json::array({10, 20, 30, 40, 50})}
    });
    json script = json::array({
        "zipWith",
        json::array({"get", json::array({"$input"}), "/array1"}),
        json::array({"get", json::array({"$input"}), "/array2"}),
        json::array({"lambda", json::array({"a", "b"}), json::array({"*", json::array({"$", "/a"}), json::array({"$", "/b"})})})
    });
    json expected = json::array({10, 40, 90}); // Only first 3 elements
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test zipWith with empty arrays
TEST_F(ComputoTest, ZipWithOperatorEmpty) {
    json input = json::object({
        {"array1", json::array()},
        {"array2", json::array({10, 20, 30})}
    });
    json script = json::array({
        "zipWith",
        json::array({"get", json::array({"$input"}), "/array1"}),
        json::array({"get", json::array({"$input"}), "/array2"}),
        json::array({"lambda", json::array({"a", "b"}), json::array({"+", json::array({"$", "/a"}), json::array({"$", "/b"})})})
    });
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test zipWith error cases
TEST_F(ComputoTest, ZipWithOperatorErrors) {
    // Wrong number of arguments
    json script1 = json::array({"zipWith", json::array({1, 2, 3})});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // First argument not an array
    json script2 = json::array({
        "zipWith",
        "not_an_array",
        json::array({1, 2, 3}),
        json::array({"lambda", json::array({"a", "b"}), json::array({"+", json::array({"$", "/a"}), json::array({"$", "/b"})})})
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Second argument not an array
    json script3 = json::array({
        "zipWith",
        json::array({1, 2, 3}),
        "not_an_array", 
        json::array({"lambda", json::array({"a", "b"}), json::array({"+", json::array({"$", "/a"}), json::array({"$", "/b"})})})
    });
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// === mapWithIndex Operator Tests ===

// Test basic mapWithIndex functionality
TEST_F(ComputoTest, MapWithIndexOperatorBasic) {
    json input = json::array({5, 7, 2, 9, 1});
    json script = json::array({
        "mapWithIndex",
        json::array({"$input"}),
        json::array({"lambda", json::array({"value", "index"}), json::array({"*", json::array({"$", "/value"}), json::array({"$", "/index"})})})
    });
    json expected = json::array({0, 7, 4, 27, 4}); // value * index
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test mapWithIndex with object construction
TEST_F(ComputoTest, MapWithIndexOperatorObjectConstruction) {
    json input = json::array({"a", "b", "c"});
    json script = json::array({
        "mapWithIndex", 
        json::array({"$input"}),
        json::array({"lambda", json::array({"value", "index"}), json::array({
            "obj",
            json::array({"value", json::array({"$", "/value"})}),
            json::array({"index", json::array({"$", "/index"})})
        })})
    });
    json expected = json::array({
        json::object({{"value", "a"}, {"index", 0}}),
        json::object({{"value", "b"}, {"index", 1}}),
        json::object({{"value", "c"}, {"index", 2}})
    });
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test mapWithIndex with empty array
TEST_F(ComputoTest, MapWithIndexOperatorEmpty) {
    json input = json::array();
    json script = json::array({
        "mapWithIndex",
        json::array({"$input"}),
        json::array({"lambda", json::array({"value", "index"}), json::array({"$", "/value"})})
    });
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test mapWithIndex error cases
TEST_F(ComputoTest, MapWithIndexOperatorErrors) {
    // Wrong number of arguments
    json script1 = json::array({"mapWithIndex", json::array({1, 2, 3})});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // First argument not an array
    json script2 = json::array({
        "mapWithIndex",
        "not_an_array",
        json::array({"lambda", json::array({"value", "index"}), json::array({"$", "/value"})})
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// === enumerate Operator Tests ===

// Test basic enumerate functionality
TEST_F(ComputoTest, EnumerateOperatorBasic) {
    json input = json::array({"apple", "banana", "cherry"});
    json script = json::array({
        "enumerate",
        json::array({"$input"})
    });
    json expected = json::array({
        json::array({0, "apple"}),
        json::array({1, "banana"}),
        json::array({2, "cherry"})
    });
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test enumerate with mixed types
TEST_F(ComputoTest, EnumerateOperatorMixedTypes) {
    json input = json::array({42, "hello", true, json(nullptr)});
    json script = json::array({
        "enumerate",
        json::array({"$input"})
    });
    json expected = json::array({
        json::array({0, 42}),
        json::array({1, "hello"}),
        json::array({2, true}),
        json::array({3, json(nullptr)})
    });
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test enumerate with empty array
TEST_F(ComputoTest, EnumerateOperatorEmpty) {
    json input = json::array();
    json script = json::array({
        "enumerate",
        json::array({"$input"})
    });
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test enumerate error cases
TEST_F(ComputoTest, EnumerateOperatorErrors) {
    // Wrong number of arguments
    json script1 = json::array({"enumerate"});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"enumerate", json::array({1, 2, 3}), "extra_arg"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Argument not an array
    json script3 = json::array({"enumerate", "not_an_array"});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// === zip Operator Tests ===

// Test basic zip functionality
TEST_F(ComputoTest, ZipOperatorBasic) {
    json input = json::object({
        {"names", json::array({"Alice", "Bob", "Charlie"})},
        {"ages", json::array({25, 30, 35})}
    });
    json script = json::array({
        "zip",
        json::array({"get", json::array({"$input"}), "/names"}),
        json::array({"get", json::array({"$input"}), "/ages"})
    });
    json expected = json::array({
        json::array({"Alice", 25}),
        json::array({"Bob", 30}),
        json::array({"Charlie", 35})
    });
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test zip with different sized arrays
TEST_F(ComputoTest, ZipOperatorDifferentSizes) {
    json input = json::object({
        {"array1", json::array({1, 2, 3, 4, 5})},
        {"array2", json::array({"a", "b", "c"})}
    });
    json script = json::array({
        "zip",
        json::array({"get", json::array({"$input"}), "/array1"}),
        json::array({"get", json::array({"$input"}), "/array2"})
    });
    json expected = json::array({
        json::array({1, "a"}),
        json::array({2, "b"}),
        json::array({3, "c"})
    });
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test zip with empty arrays
TEST_F(ComputoTest, ZipOperatorEmpty) {
    json input = json::object({
        {"array1", json::array()},
        {"array2", json::array({1, 2, 3})}
    });
    json script = json::array({
        "zip",
        json::array({"get", json::array({"$input"}), "/array1"}),
        json::array({"get", json::array({"$input"}), "/array2"})
    });
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test zip error cases
TEST_F(ComputoTest, ZipOperatorErrors) {
    // Wrong number of arguments
    json script1 = json::array({"zip", json::array({1, 2, 3})});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // First argument not an array
    json script2 = json::array({"zip", "not_an_array", json::array({1, 2, 3})});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Second argument not an array
    json script3 = json::array({"zip", json::array({1, 2, 3}), "not_an_array"});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// === Integration Tests for Multi-Parameter Lambda Features ===

// Test complex combination: enumerate + mapWithIndex + zipWith
TEST_F(ComputoTest, MultiParamLambdaIntegration) {
    json input = json::array({10, 20, 30});
    json script = json::array({
        "zipWith",
        json::array({
            "enumerate",
            json::array({"$input"})
        }),
        json::array({
            "mapWithIndex",
            json::array({"$input"}),
            json::array({"lambda", json::array({"value", "index"}), json::array({"+", json::array({"$", "/value"}), json::array({"$", "/index"})})})
        }),
        json::array({"lambda", json::array({"enumerated", "mapped"}), json::array({
            "obj",
            json::array({"index", json::array({"get", json::array({"$", "/enumerated"}), "/0"})}),
            json::array({"original", json::array({"get", json::array({"$", "/enumerated"}), "/1"})}),
            json::array({"calculated", json::array({"$", "/mapped"})})
        })})
    });
    
    json expected = json::array({
        json::object({{"index", 0}, {"original", 10}, {"calculated", 10}}),  // 10 + 0 = 10
        json::object({{"index", 1}, {"original", 20}, {"calculated", 21}}),  // 20 + 1 = 21
        json::object({{"index", 2}, {"original", 30}, {"calculated", 32}})   // 30 + 2 = 32
    });
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test reduce with complex multi-parameter lambda
TEST_F(ComputoTest, ReduceWithComplexMultiParamLambda) {
    json input = json::array({1, 2, 3, 4, 5});
    json script = json::array({
        "reduce",
        json::array({"$input"}),
        json::array({"lambda", json::array({"acc", "item"}), json::array({
            "obj",
            json::array({"sum", json::array({"+", json::array({"get", json::array({"$", "/acc"}), "/sum"}), json::array({"$", "/item"})})}),
            json::array({"count", json::array({"+", json::array({"get", json::array({"$", "/acc"}), "/count"}), 1})}),
            json::array({"max", json::array({"if", json::array({">", json::array({"$", "/item"}), json::array({"get", json::array({"$", "/acc"}), "/max"})}), json::array({"$", "/item"}), json::array({"get", json::array({"$", "/acc"}), "/max"})})})
        })}),
        json::object({{"sum", 0}, {"count", 0}, {"max", 0}})
    });
    
    json expected = json::object({{"sum", 15}, {"count", 5}, {"max", 5}});
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test lambda variable resolution with multi-parameter lambdas
TEST_F(ComputoTest, MultiParamLambdaVariableResolution) {
    json input = json::array({1, 2, 3});
    json script = json::array({
        "let",
        json::array({json::array({
            "adder",
            json::array({"lambda", json::array({"a", "b"}), json::array({"+", json::array({"$", "/a"}), json::array({"$", "/b"})})})
        })}),
        json::array({
            "zipWith",
            json::array({"$input"}),
            json{{"array", json::array({10, 20, 30})}},
            json::array({"$", "/adder"})
        })
    });
    
    json expected = json::array({11, 22, 33});
    EXPECT_EQ(computo::execute(script, input), expected);
}

// Test error handling in complex multi-parameter scenarios
TEST_F(ComputoTest, MultiParamLambdaErrorHandling) {
    json input = json::array({1, 2, 3});
    
    // Test lambda parameter count mismatch in zipWith
    json script = json::array({
        "zipWith",
        json::array({"$input"}),
        json::array({10, 20, 30}),
        json::array({"lambda", json::array({"a"}), json::array({"$", "/a"})}) // Only 1 param but zipWith provides 2
    });
    
    EXPECT_THROW(computo::execute(script, input), computo::InvalidArgumentException);
}

// === concat operator tests ===

// Test basic string concatenation
TEST_F(ComputoTest, ConcatOperatorBasic) {
    json script = json::array({"concat", "Hello", " ", "World"});
    EXPECT_EQ(computo::execute(script, input_data), "Hello World");
}

// Test concatenation with single argument
TEST_F(ComputoTest, ConcatOperatorSingle) {
    json script = json::array({"concat", "Hello"});
    EXPECT_EQ(computo::execute(script, input_data), "Hello");
}

// Test concatenation with many arguments
TEST_F(ComputoTest, ConcatOperatorMany) {
    json script = json::array({"concat", "a", "b", "c", "d", "e"});
    EXPECT_EQ(computo::execute(script, input_data), "abcde");
}

// Test concatenation with empty strings
TEST_F(ComputoTest, ConcatOperatorEmptyStrings) {
    json script = json::array({"concat", "", "Hello", "", "World", ""});
    EXPECT_EQ(computo::execute(script, input_data), "HelloWorld");
}

// Test concatenation with numbers
TEST_F(ComputoTest, ConcatOperatorNumbers) {
    json script = json::array({"concat", "Count: ", 42, " items"});
    EXPECT_EQ(computo::execute(script, input_data), "Count: 42 items");
}

// Test concatenation with floating point numbers
TEST_F(ComputoTest, ConcatOperatorFloats) {
    json script = json::array({"concat", "Pi is ", 3.14159});
    EXPECT_EQ(computo::execute(script, input_data), "Pi is 3.14159");
}

// Test concatenation with booleans
TEST_F(ComputoTest, ConcatOperatorBooleans) {
    json script = json::array({"concat", "Success: ", true, ", Failed: ", false});
    EXPECT_EQ(computo::execute(script, input_data), "Success: true, Failed: false");
}

// Test concatenation with null values
TEST_F(ComputoTest, ConcatOperatorNull) {
    json script = json::array({"concat", "Before", json(nullptr), "After"});
    EXPECT_EQ(computo::execute(script, input_data), "BeforeAfter");
}

// Test concatenation with mixed types
TEST_F(ComputoTest, ConcatOperatorMixed) {
    json script = json::array({"concat", "User ", 123, " is ", true, " years old: ", 25.5});
    EXPECT_EQ(computo::execute(script, input_data), "User 123 is true years old: 25.5");
}

// Test concatenation with arrays and objects (JSON representation)
TEST_F(ComputoTest, ConcatOperatorComplexTypes) {
    json script = json::array({
        "concat", 
        "Array: ", 
        json{{"array", json::array({1, 2, 3})}},
        " Object: ",
        json{{"name", "test"}}
    });
    EXPECT_EQ(computo::execute(script, input_data), "Array: [1,2,3] Object: {\"name\":\"test\"}");
}

// Test concatenation with expressions
TEST_F(ComputoTest, ConcatOperatorWithExpressions) {
    json script = json::array({
        "concat",
        "Sum: ",
        json::array({"+", 2, 3}),
        " Input: ",
        json::array({"get", json::array({"$input"}), "/test"})
    });
    EXPECT_EQ(computo::execute(script, input_data), "Sum: 5 Input: value");
}

// Test concatenation with let variables
TEST_F(ComputoTest, ConcatOperatorWithLet) {
    json script = json::array({
        "let",
        json::array({
            json::array({"name", "Alice"}),
            json::array({"age", 30})
        }),
        json::array({"concat", "User ", json::array({"$", "/name"}), " is ", json::array({"$", "/age"}), " years old"})
    });
    EXPECT_EQ(computo::execute(script, input_data), "User Alice is 30 years old");
}

// Test concatenation in object construction
TEST_F(ComputoTest, ConcatOperatorInObject) {
    json script = json::array({
        "obj",
        json::array({"message", json::array({"concat", "Hello ", "World"})}),
        json::array({"count", json::array({"concat", "Total: ", 42})})
    });
    json expected = json{{"message", "Hello World"}, {"count", "Total: 42"}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test concatenation with map operation
TEST_F(ComputoTest, ConcatOperatorWithMap) {
    json script = json::array({
        "map",
        json{{"array", json::array({"Alice", "Bob", "Charlie"})}},
        json::array({"lambda", json::array({"name"}), json::array({"concat", "Hello, ", json::array({"$", "/name"}), "!"})})
    });
    json expected = json::array({"Hello, Alice!", "Hello, Bob!", "Hello, Charlie!"});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test nested concatenation
TEST_F(ComputoTest, ConcatOperatorNested) {
    json script = json::array({
        "concat",
        json::array({"concat", "A", "B"}),
        json::array({"concat", "C", "D"}),
        "E"
    });
    EXPECT_EQ(computo::execute(script, input_data), "ABCDE");
}

// Test concatenation with $input
TEST_F(ComputoTest, ConcatOperatorWithInput) {
    json test_input = json{{"user", "John"}, {"action", "login"}};
    json script = json::array({
        "concat",
        "User ",
        json::array({"get", json::array({"$input"}), "/user"}),
        " performed action: ",
        json::array({"get", json::array({"$input"}), "/action"})
    });
    EXPECT_EQ(computo::execute(script, test_input), "User John performed action: login");
}

// Test concatenation with complex nested objects
TEST_F(ComputoTest, ConcatOperatorComplexObjects) {
    json nested_obj = json{
        {"level1", json{
            {"level2", json{
                {"data", json::array({1, 2, 3})},
                {"info", "test"}
            }}
        }}
    };
    json script = json::array({"concat", "Data: ", nested_obj});
    std::string expected_start = "Data: ";
    auto result = computo::execute(script, input_data).get<std::string>();
    EXPECT_TRUE(result.substr(0, expected_start.length()) == expected_start);
    EXPECT_TRUE(result.find("level1") != std::string::npos);
    EXPECT_TRUE(result.find("level2") != std::string::npos);
}

// Test concatenation with reduce operation
TEST_F(ComputoTest, ConcatOperatorWithReduce) {
    json script = json::array({
        "reduce",
        json{{"array", json::array({"a", "b", "c", "d"})}},
        json::array({"lambda", json::array({"acc", "item"}), json::array({"concat", json::array({"$", "/acc"}), json::array({"$", "/item"})})}),
        ""
    });
    EXPECT_EQ(computo::execute(script, input_data), "abcd");
}

// Test concatenation with if operator
TEST_F(ComputoTest, ConcatOperatorWithIf) {
    json script = json::array({
        "if",
        true,
        json::array({"concat", "Success: ", "Operation completed"}),
        json::array({"concat", "Error: ", "Operation failed"})
    });
    EXPECT_EQ(computo::execute(script, input_data), "Success: Operation completed");
}

// === concat operator error tests ===

// Test concatenation with no arguments
TEST_F(ComputoTest, ConcatOperatorNoArguments) {
    json script = json::array({"concat"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test concatenation error message content
TEST_F(ComputoTest, ConcatOperatorErrorMessage) {
    json script = json::array({"concat"});
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg = e.what();
        EXPECT_NE(error_msg.find("concat operator requires at least 1 argument"), std::string::npos);
    }
}

// Test obj operator with computed keys
TEST_F(ComputoTest, ObjOperatorComputedKeys) {
    // ["let", [["name", "Alice"], ["age", 50]], ["obj", [["$", "/name"], ["$", "/age"]]]]
    json script = json::array({
        "let",
        json::array({
            json::array({"name", "Alice"}),
            json::array({"age", 50})
        }),
        json::array({
            "obj",
            json::array({json::array({"$", "/name"}), json::array({"$", "/age"})})
        })
    });
    json expected = json{{"Alice", 50}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test obj operator with mixed literal and computed keys
TEST_F(ComputoTest, ObjOperatorMixedKeys) {
    // ["let", [["dynamic_key", "computed"]], ["obj", ["literal", "value1"], [["$", "/dynamic_key"], "value2"]]]
    json script = json::array({
        "let",
        json::array({json::array({"dynamic_key", "computed"})}),
        json::array({
            "obj",
            json::array({"literal", "value1"}),
            json::array({json::array({"$", "/dynamic_key"}), "value2"})
        })
    });
    json expected = json{{"literal", "value1"}, {"computed", "value2"}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test obj operator with computed key expression
TEST_F(ComputoTest, ObjOperatorComputedKeyExpression) {
    // ["let", [["prefix", "user_"], ["id", 123]], ["obj", [["concat", ["$", "/prefix"], ["$", "/id"]], "John"]]]
    json script = json::array({
        "let",
        json::array({
            json::array({"prefix", "user_"}),
            json::array({"id", 123})
        }),
        json::array({
            "obj",
            json::array({
                json::array({"concat", json::array({"$", "/prefix"}), json::array({"$", "/id"})}),
                "John"
            })
        })
    });
    json expected = json{{"user_123", "John"}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// === Tests for N-ary Operators ===

// === Addition (n-ary) Tests ===

TEST_F(ComputoTest, AdditionNaryBasic) {
    // Test 3 arguments
    json script = json::array({"+", 1, 2, 3});
    EXPECT_EQ(computo::execute(script, input_data), 6);
    
    // Test 4 arguments
    json script2 = json::array({"+", 10, 20, 30, 40});
    EXPECT_EQ(computo::execute(script2, input_data), 100);
    
    // Test 5 arguments with mixed integers
    json script3 = json::array({"+", 1, 2, 3, 4, 5});
    EXPECT_EQ(computo::execute(script3, input_data), 15);
}

TEST_F(ComputoTest, AdditionNaryFloats) {
    // Test with floating point numbers
    json script = json::array({"+", 1.5, 2.5, 3.0});
    EXPECT_EQ(computo::execute(script, input_data), 7.0);
    
    // Test mixed integers and floats
    json script2 = json::array({"+", 1, 2.5, 3});
    EXPECT_EQ(computo::execute(script2, input_data), 6.5);
}

TEST_F(ComputoTest, AdditionNarySingleArgument) {
    // Single argument should return that argument
    json script = json::array({"+", 42});
    EXPECT_EQ(computo::execute(script, input_data), 42);
    
    json script2 = json::array({"+", 3.14});
    EXPECT_EQ(computo::execute(script2, input_data), 3.14);
}

TEST_F(ComputoTest, AdditionNaryLargeNumber) {
    // Test with many arguments
    json script = json::array({"+", 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}); // 10 ones
    EXPECT_EQ(computo::execute(script, input_data), 10);
}

TEST_F(ComputoTest, AdditionNaryWithExpressions) {
    // Test with expressions as arguments
    json script = json::array({
        "+", 
        json::array({"+", 1, 2}),  // 3
        json::array({"*", 2, 3}),  // 6
        5                          // 5
    });
    EXPECT_EQ(computo::execute(script, input_data), 14);
}

// === Multiplication (n-ary) Tests ===

TEST_F(ComputoTest, MultiplicationNaryBasic) {
    // Test 3 arguments
    json script = json::array({"*", 2, 3, 4});
    EXPECT_EQ(computo::execute(script, input_data), 24);
    
    // Test 4 arguments
    json script2 = json::array({"*", 1, 2, 3, 4});
    EXPECT_EQ(computo::execute(script2, input_data), 24);
}

TEST_F(ComputoTest, MultiplicationNaryFloats) {
    // Test with floating point numbers
    json script = json::array({"*", 2.0, 3.0, 1.5});
    EXPECT_EQ(computo::execute(script, input_data), 9.0);
    
    // Test mixed integers and floats
    json script2 = json::array({"*", 2, 2.5, 2});
    EXPECT_EQ(computo::execute(script2, input_data), 10.0);
}

TEST_F(ComputoTest, MultiplicationNarySingleArgument) {
    // Single argument should return that argument
    json script = json::array({"*", 7});
    EXPECT_EQ(computo::execute(script, input_data), 7);
    
    json script2 = json::array({"*", 2.5});
    EXPECT_EQ(computo::execute(script2, input_data), 2.5);
}

TEST_F(ComputoTest, MultiplicationNaryWithZero) {
    // Test with zero (should result in zero)
    json script = json::array({"*", 5, 0, 10});
    EXPECT_EQ(computo::execute(script, input_data), 0);
}

TEST_F(ComputoTest, MultiplicationNaryWithExpressions) {
    // Test with expressions as arguments
    json script = json::array({
        "*", 
        json::array({"+", 1, 1}),  // 2
        json::array({"+", 2, 1}),  // 3
        json::array({"+", 1, 3})   // 4
    });
    EXPECT_EQ(computo::execute(script, input_data), 24);
}

// === Chained Comparison Tests ===

TEST_F(ComputoTest, ChainedLessThanBasic) {
    // Test a < b < c (true case)
    json script = json::array({"<", 1, 2, 3});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test a < b < c (false case)
    json script2 = json::array({"<", 1, 3, 2});
    EXPECT_EQ(computo::execute(script2, input_data), false);
    
    // Test longer chain
    json script3 = json::array({"<", 1, 2, 3, 4, 5});
    EXPECT_EQ(computo::execute(script3, input_data), true);
}

TEST_F(ComputoTest, ChainedGreaterThanBasic) {
    // Test a > b > c (true case)
    json script = json::array({">", 5, 3, 1});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test a > b > c (false case)
    json script2 = json::array({">", 5, 1, 3});
    EXPECT_EQ(computo::execute(script2, input_data), false);
    
    // Test longer chain
    json script3 = json::array({">", 10, 8, 6, 4, 2});
    EXPECT_EQ(computo::execute(script3, input_data), true);
}

TEST_F(ComputoTest, ChainedLessEqualBasic) {
    // Test a <= b <= c (true case with equality)
    json script = json::array({"<=", 1, 2, 2});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test a <= b <= c (true case strictly increasing)
    json script2 = json::array({"<=", 1, 2, 3});
    EXPECT_EQ(computo::execute(script2, input_data), true);
    
    // Test a <= b <= c (false case)
    json script3 = json::array({"<=", 3, 2, 4});
    EXPECT_EQ(computo::execute(script3, input_data), false);
}

TEST_F(ComputoTest, ChainedGreaterEqualBasic) {
    // Test a >= b >= c (true case with equality)
    json script = json::array({">=", 3, 3, 2});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test a >= b >= c (true case strictly decreasing)
    json script2 = json::array({">=", 5, 3, 1});
    EXPECT_EQ(computo::execute(script2, input_data), true);
    
    // Test a >= b >= c (false case)
    json script3 = json::array({">=", 2, 4, 1});
    EXPECT_EQ(computo::execute(script3, input_data), false);
}

TEST_F(ComputoTest, ChainedComparisonFloats) {
    // Test with floating point numbers
    json script = json::array({"<", 1.1, 2.2, 3.3});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    json script2 = json::array({">", 5.5, 4.4, 3.3});
    EXPECT_EQ(computo::execute(script2, input_data), true);
}

TEST_F(ComputoTest, ChainedComparisonWithExpressions) {
    // Test with expressions
    json script = json::array({
        "<",
        json::array({"+", 1, 1}),  // 2
        json::array({"*", 2, 2}),  // 4
        json::array({"+", 3, 3})   // 6
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

TEST_F(ComputoTest, ChainedComparisonMinimal) {
    // Test with just 2 arguments (should work like binary)
    json script = json::array({"<", 1, 2});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    json script2 = json::array({">", 5, 3});
    EXPECT_EQ(computo::execute(script2, input_data), true);
}

// === N-ary Equality Tests ===

TEST_F(ComputoTest, EqualityNaryBasic) {
    // Test all equal (true case)
    json script = json::array({"==", 5, 5, 5});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test one different (false case)
    json script2 = json::array({"==", 5, 5, 6});
    EXPECT_EQ(computo::execute(script2, input_data), false);
    
    // Test longer chain of equal values
    json script3 = json::array({"==", 42, 42, 42, 42, 42});
    EXPECT_EQ(computo::execute(script3, input_data), true);
}

TEST_F(ComputoTest, EqualityNaryStrings) {
    // Test with strings
    json script = json::array({"==", "hello", "hello", "hello"});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    json script2 = json::array({"==", "hello", "hello", "world"});
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

TEST_F(ComputoTest, EqualityNaryMixedTypes) {
    // Test with same value, different types (in JSON, 5 and 5.0 are actually equal)
    json script = json::array({"==", 5, 5.0});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test different types that are actually different
    json script2 = json::array({"==", 5, "5"});
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

TEST_F(ComputoTest, EqualityNaryWithExpressions) {
    // Test with expressions that evaluate to same value
    json script = json::array({
        "==",
        json::array({"+", 2, 3}),  // 5
        json::array({"*", 1, 5}),  // 5
        json::array({"-", 7, 2})   // 5
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

TEST_F(ComputoTest, EqualityNaryMinimal) {
    // Test with just 2 arguments (should work like binary)
    json script = json::array({"==", 5, 5});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    json script2 = json::array({"==", 5, 6});
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

// === Error Cases for N-ary Operators ===

TEST_F(ComputoTest, AdditionNaryNoArguments) {
    // Should still throw error for no arguments
    json script = json::array({"+"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, MultiplicationNaryNoArguments) {
    // Should still throw error for no arguments
    json script = json::array({"*"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, AdditionNaryNonNumeric) {
    // Should throw error for non-numeric arguments
    json script = json::array({"+", 1, "hello", 3});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, MultiplicationNaryNonNumeric) {
    // Should throw error for non-numeric arguments
    json script = json::array({"*", 2, true, 4});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, ChainedComparisonTooFewArgs) {
    // Should throw error for less than 2 arguments
    json script = json::array({"<", 5});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({">", 5});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, ChainedComparisonNonNumeric) {
    // Should throw error for non-numeric arguments
    json script = json::array({"<", 1, "hello", 3});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(ComputoTest, EqualityNaryTooFewArgs) {
    // Should throw error for less than 2 arguments
    json script = json::array({"==", 5});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// === Integration Tests for N-ary Operators ===

TEST_F(ComputoTest, NaryOperatorsWithLet) {
    // Test n-ary operators within let expressions
    json script = json::array({
        "let",
        json::array({
            json::array({"a", 1}),
            json::array({"b", 2}),
            json::array({"c", 3}),
            json::array({"d", 4})
        }),
        json::array({
            "+",
            json::array({"$", "/a"}),
            json::array({"$", "/b"}),
            json::array({"$", "/c"}),
            json::array({"$", "/d"})
        })
    });
    EXPECT_EQ(computo::execute(script, input_data), 10);
}

TEST_F(ComputoTest, NaryOperatorsInArray) {
    // Test n-ary operators in array construction
    json script = json::array({
        "obj",
        json::array({"sum", json::array({"+", 1, 2, 3, 4})}),
        json::array({"product", json::array({"*", 2, 3, 4})}),
        json::array({"chain_check", json::array({"<", 1, 2, 3, 4})})
    });
    
    json result = computo::execute(script, input_data);
    EXPECT_EQ(result["sum"], 10);
    EXPECT_EQ(result["product"], 24);
    EXPECT_EQ(result["chain_check"], true);
}

TEST_F(ComputoTest, NaryOperatorsWithMap) {
    // Test n-ary operators with map
    json script = json::array({
        "map",
        json{{"array", json::array({
            json{{"array", json::array({1, 2, 3})}},
            json{{"array", json::array({2, 3, 4})}},
            json{{"array", json::array({5, 6, 7})}}
        })}},
        json::array({
            "lambda",
            json::array({"arr"}),
            json::array({
                "+",
                json::array({"get", json::array({"$", "/arr"}), "/0"}),
                json::array({"get", json::array({"$", "/arr"}), "/1"}),
                json::array({"get", json::array({"$", "/arr"}), "/2"})
            })
        })
    });
    
    json expected = json::array({6, 9, 18});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

TEST_F(ComputoTest, NaryOperatorsPerformanceStressTest) {
    // Test with many arguments to ensure no significant performance degradation
    nlohmann::json args = json::array({"+"});
    for (int i = 1; i <= 100; ++i) {
        args.push_back(i);
    }
    
    json result = computo::execute(args, input_data);
    EXPECT_EQ(result, 5050); // Sum of 1 to 100
}

TEST_F(ComputoTest, ChainedComparisonComplexUsage) {
    // Test chained comparison in realistic scenario
    json script = json::array({
        "let",
        json::array({json::array({"score", 85})}),
        json::array({
            "if",
            json::array({"<", 80, json::array({"$", "/score"}), 90}),  // 80 < score < 90
            "B grade",
            "Other grade"
        })
    });
    
    EXPECT_EQ(computo::execute(script, input_data), "B grade");
}

// === Edge Cases and Advanced Scenarios ===

TEST_F(ComputoTest, MixedNaryAndBinaryOperators) {
    // Test mixing n-ary and binary operators
    json script = json::array({
        "+",
        json::array({"*", 2, 3}),      // 6 (binary multiplication)
        json::array({"*", 1, 2, 3}),   // 6 (n-ary multiplication)
        json::array({"+", 1, 1})       // 2 (binary addition)
    });
    EXPECT_EQ(computo::execute(script, input_data), 14);
}

TEST_F(ComputoTest, NestedNaryOperators) {
    // Test deeply nested n-ary operators
    json script = json::array({
        "*",
        json::array({"+", 1, 1, 1}),           // 3
        json::array({"+", 2, 2}),               // 4
        json::array({"*", 1, 2, 3})             // 6
    });
    EXPECT_EQ(computo::execute(script, input_data), 72); // 3 * 4 * 6
}
