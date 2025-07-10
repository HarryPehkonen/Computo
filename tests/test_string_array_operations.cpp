#include <gtest/gtest.h>
#include <computo.hpp>
using json = nlohmann::json;

class StringArrayOperationsTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

// Test split operator
TEST_F(StringArrayOperationsTest, SplitOperatorBasic) {
    json script = json::array({"split", "hello world", " "});
    json expected = json::object({{"array", json::array({"hello", "world"})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorMultipleDelimiters) {
    json script = json::array({"split", "a,b,c", ","});
    json expected = json::object({{"array", json::array({"a", "b", "c"})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorEmptyDelimiter) {
    json script = json::array({"split", "abc", ""});
    json expected = json::object({{"array", json::array({"a", "b", "c"})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorEmptyString) {
    json script = json::array({"split", "", ","});
    json expected = json::object({{"array", json::array({""})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorNoDelimiterFound) {
    json script = json::array({"split", "hello", ","});
    json expected = json::object({{"array", json::array({"hello"})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({"split", "hello"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"split", 123, " "})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"split", "hello", 123})), computo::InvalidArgumentException);
}

// Test join operator
TEST_F(StringArrayOperationsTest, JoinOperatorBasic) {
    json script = json::array({"join", 
        json::object({{"array", json::array({"hello", "world"})}}), 
        " "
    });
    EXPECT_EQ(exec(script), "hello world");
}

TEST_F(StringArrayOperationsTest, JoinOperatorEmptySeparator) {
    json script = json::array({"join", 
        json::object({{"array", json::array({"a", "b", "c"})}}), 
        ""
    });
    EXPECT_EQ(exec(script), "abc");
}

TEST_F(StringArrayOperationsTest, JoinOperatorEmptyArray) {
    json script = json::array({"join", 
        json::object({{"array", json::array()}}), 
        ","
    });
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, JoinOperatorMixedTypes) {
    json script = json::array({"join", 
        json::object({{"array", json::array({"hello", 42, true, json(nullptr)})}}), 
        ","
    });
    EXPECT_EQ(exec(script), "hello,42,true,null");
}

TEST_F(StringArrayOperationsTest, JoinOperatorComplexElements) {
    json script = json::array({"join", 
        json::object({{"array", json::array({
            json::array({1, 2, 3}),
            json{{"key", "value"}}
        })}}), 
        " | "
    });
    EXPECT_EQ(exec(script), "[1,2,3] | {\"key\":\"value\"}");
}

TEST_F(StringArrayOperationsTest, JoinOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({"join", json::object({{"array", json::array({"a"})}})})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"join", "not_an_array", ","})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"join", json::object({{"array", json::array({"a"})}}), 123})), computo::InvalidArgumentException);
}

// Test trim operator
TEST_F(StringArrayOperationsTest, TrimOperatorBasic) {
    json script = json::array({"trim", "  hello world  "});
    EXPECT_EQ(exec(script), "hello world");
}

TEST_F(StringArrayOperationsTest, TrimOperatorAllWhitespace) {
    json script = json::array({"trim", "   \t\n\r\f\v   "});
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, TrimOperatorNoWhitespace) {
    json script = json::array({"trim", "hello"});
    EXPECT_EQ(exec(script), "hello");
}

TEST_F(StringArrayOperationsTest, TrimOperatorEmptyString) {
    json script = json::array({"trim", ""});
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, TrimOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({"trim"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"trim", 123})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"trim", "hello", "world"})), computo::InvalidArgumentException);
}

// Test upper operator
TEST_F(StringArrayOperationsTest, UpperOperatorBasic) {
    json script = json::array({"upper", "Hello World"});
    EXPECT_EQ(exec(script), "HELLO WORLD");
}

TEST_F(StringArrayOperationsTest, UpperOperatorAlreadyUpper) {
    json script = json::array({"upper", "HELLO"});
    EXPECT_EQ(exec(script), "HELLO");
}

TEST_F(StringArrayOperationsTest, UpperOperatorMixedCase) {
    json script = json::array({"upper", "HeLLo WoRLD"});
    EXPECT_EQ(exec(script), "HELLO WORLD");
}

TEST_F(StringArrayOperationsTest, UpperOperatorEmptyString) {
    json script = json::array({"upper", ""});
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, UpperOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({"upper"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"upper", 123})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"upper", "hello", "world"})), computo::InvalidArgumentException);
}

// Test lower operator
TEST_F(StringArrayOperationsTest, LowerOperatorBasic) {
    json script = json::array({"lower", "Hello World"});
    EXPECT_EQ(exec(script), "hello world");
}

TEST_F(StringArrayOperationsTest, LowerOperatorAlreadyLower) {
    json script = json::array({"lower", "hello"});
    EXPECT_EQ(exec(script), "hello");
}

TEST_F(StringArrayOperationsTest, LowerOperatorMixedCase) {
    json script = json::array({"lower", "HeLLo WoRLD"});
    EXPECT_EQ(exec(script), "hello world");
}

TEST_F(StringArrayOperationsTest, LowerOperatorEmptyString) {
    json script = json::array({"lower", ""});
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, LowerOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({"lower"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"lower", 123})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"lower", "hello", "world"})), computo::InvalidArgumentException);
}

// Test sort operator - updated for new API
TEST_F(StringArrayOperationsTest, SortOperatorBasic) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({"charlie", "alice", "bob"})}})
    });
    json expected = json::object({{"array", json::array({"alice", "bob", "charlie"})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorNumbers) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({3, 1, 2})}})
    });
    json expected = json::object({{"array", json::array({1, 2, 3})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorDescending) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({3, 1, 2})}}),
        "desc"
    });
    json expected = json::object({{"array", json::array({3, 2, 1})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorAscendingExplicit) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({"charlie", "alice", "bob"})}}),
        "asc"
    });
    json expected = json::object({{"array", json::array({"alice", "bob", "charlie"})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorTypeAware) {
    // Type ordering: null < numbers < strings < booleans < arrays < objects
    json script = json::array({"sort", 
        json::object({{"array", json::array({"b", 2, nullptr, true, json::array({1}), "a", 1})}})
    });
    auto result = exec(script);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    auto arr = result["array"];
    ASSERT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), 7);
    // Check ordering: null first, then numbers, then strings, then booleans, then arrays
    EXPECT_TRUE(arr[0].is_null());
    EXPECT_TRUE(arr[1].is_number());
    EXPECT_TRUE(arr[2].is_number());
    EXPECT_TRUE(arr[3].is_string());
    EXPECT_TRUE(arr[4].is_string());
    EXPECT_TRUE(arr[5].is_boolean());
    EXPECT_TRUE(arr[6].is_array());
}

TEST_F(StringArrayOperationsTest, SortOperatorObjectSingleField) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({
            json::object({{"name", "charlie"}, {"age", 30}}),
            json::object({{"name", "alice"}, {"age", 25}}),
            json::object({{"name", "bob"}, {"age", 35}})
        })}}),
        "/name"
    });
    auto result = exec(script);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    auto arr = result["array"];
    ASSERT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0]["name"], "alice");
    EXPECT_EQ(arr[1]["name"], "bob");
    EXPECT_EQ(arr[2]["name"], "charlie");
}

TEST_F(StringArrayOperationsTest, SortOperatorObjectSingleFieldDesc) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({
            json::object({{"name", "charlie"}, {"age", 30}}),
            json::object({{"name", "alice"}, {"age", 25}}),
            json::object({{"name", "bob"}, {"age", 35}})
        })}}),
        json::array({"/name", "desc"})
    });
    auto result = exec(script);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    auto arr = result["array"];
    ASSERT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0]["name"], "charlie");
    EXPECT_EQ(arr[1]["name"], "bob");
    EXPECT_EQ(arr[2]["name"], "alice");
}

TEST_F(StringArrayOperationsTest, SortOperatorObjectMultiField) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({
            json::object({{"name", "alice"}, {"age", 30}}),
            json::object({{"name", "bob"}, {"age", 25}}),
            json::object({{"name", "alice"}, {"age", 25}})
        })}}),
        "/name",
        json::array({"/age", "desc"})
    });
    auto result = exec(script);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    auto arr = result["array"];
    ASSERT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), 3);
    // First alice (age 30), then alice (age 25), then bob (age 25)
    EXPECT_EQ(arr[0]["name"], "alice");
    EXPECT_EQ(arr[0]["age"], 30);
    EXPECT_EQ(arr[1]["name"], "alice");
    EXPECT_EQ(arr[1]["age"], 25);
    EXPECT_EQ(arr[2]["name"], "bob");
    EXPECT_EQ(arr[2]["age"], 25);
}

TEST_F(StringArrayOperationsTest, SortOperatorEmptyArray) {
    json script = json::array({"sort", 
        json::object({{"array", json::array()}})
    });
    json expected = json::object({{"array", json::array()}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({"sort"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"sort", "not_an_array"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"sort", json::object({{"array", json::array({1, 2, 3})}}), "invalid_direction"})), computo::InvalidArgumentException);
}

// Test reverse operator
TEST_F(StringArrayOperationsTest, ReverseOperatorBasic) {
    json script = json::array({"reverse", 
        json::object({{"array", json::array({1, 2, 3, 4})}})
    });
    json expected = json::object({{"array", json::array({4, 3, 2, 1})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, ReverseOperatorStrings) {
    json script = json::array({"reverse", 
        json::object({{"array", json::array({"a", "b", "c"})}})
    });
    json expected = json::object({{"array", json::array({"c", "b", "a"})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, ReverseOperatorEmptyArray) {
    json script = json::array({"reverse", 
        json::object({{"array", json::array()}})
    });
    json expected = json::object({{"array", json::array()}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, ReverseOperatorSingleElement) {
    json script = json::array({"reverse", 
        json::object({{"array", json::array({"only"})}})
    });
    json expected = json::object({{"array", json::array({"only"})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, ReverseOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({"reverse"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"reverse", "not_an_array"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"reverse", json::object({{"array", json::array({"a"})}}), "extra"})), computo::InvalidArgumentException);
}

// Test unique operator
// Test enhanced unique operator - requires pre-sorted data
TEST_F(StringArrayOperationsTest, UniqueOperatorBasicFirsts) {
    // Pre-sorted array for unique processing: ["a", "a", "b", "c", "c"]
    json script = json::array({"unique", 
        json::object({{"array", json::array({"a", "a", "b", "c", "c"})}})
    });
    json expected = json::object({{"array", json::array({"a", "b", "c"})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorLasts) {
    json script = json::array({"unique", 
        json::object({{"array", json::array({1, 1, 2, 3, 3, 3})}}),
        "lasts"
    });
    json expected = json::object({{"array", json::array({1, 2, 3})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorSingles) {
    json script = json::array({"unique", 
        json::object({{"array", json::array({1, 1, 2, 3, 3, 3})}}),
        "singles"
    });
    json expected = json::object({{"array", json::array({2})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorMultiples) {
    json script = json::array({"unique", 
        json::object({{"array", json::array({1, 1, 2, 3, 3, 3})}}),
        "multiples"
    });
    json expected = json::object({{"array", json::array({1, 3})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorObjectByField) {
    json script = json::array({"unique", 
        json::object({{"array", json::array({
            json::object({{"name", "alice"}, {"dept", "eng"}}),
            json::object({{"name", "alice"}, {"dept", "sales"}}),
            json::object({{"name", "bob"}, {"dept", "hr"}}),
            json::object({{"name", "charlie"}, {"dept", "eng"}})
        })}}),
        "/name"
    });
    auto result = exec(script);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    auto arr = result["array"];
    ASSERT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0]["name"], "alice");
    EXPECT_EQ(arr[1]["name"], "bob");
    EXPECT_EQ(arr[2]["name"], "charlie");
}

TEST_F(StringArrayOperationsTest, UniqueOperatorObjectFieldSingles) {
    json script = json::array({"unique", 
        json::object({{"array", json::array({
            json::object({{"dept", "eng"}}),
            json::object({{"dept", "eng"}}),
            json::object({{"dept", "hr"}}),
            json::object({{"dept", "sales"}}),
            json::object({{"dept", "sales"}})
        })}}),
        "/dept",
        "singles"
    });
    auto result = exec(script);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    auto arr = result["array"];
    ASSERT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), 1);
    EXPECT_EQ(arr[0]["dept"], "hr");
}

TEST_F(StringArrayOperationsTest, UniqueOperatorEmptyArray) {
    json script = json::array({"unique", 
        json::object({{"array", json::array()}})
    });
    json expected = json::object({{"array", json::array()}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorSingleElement) {
    json script = json::array({"unique", 
        json::object({{"array", json::array({42})}}),
        "singles"
    });
    json expected = json::object({{"array", json::array({42})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({"unique"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"unique", "not_an_array"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"unique", json::object({{"array", json::array({"a"})}}), "invalid_mode"})), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({"unique", json::object({{"array", json::array({"a"})}}), "/field", "invalid_mode"})), computo::InvalidArgumentException);
}

// Test complex scenarios
TEST_F(StringArrayOperationsTest, ComplexStringProcessing) {
    json script = json::array({
        "let",
        json::array({
            json::array({"text", "  Hello, World! How are you?  "})
        }),
        json::array({
            "join",
            json::array({
                "reverse",
                json::array({
                    "unique",
                    json::array({
                        "sort",
                        json::array({
                            "map",
                            json::array({
                                "split",
                                json::array({
                                    "trim",
                                    json::array({"$", "/text"})
                                }),
                                " "
                            }),
                            json::array({"lambda", json::array({"word"}), json::array({"lower", json::array({"$", "/word"})})})
                        })
                    })
                })
            }),
            " "
        })
    });
    
    // Should: trim -> split -> map(lower) -> sort -> unique -> reverse -> join
    // "Hello, World! How are you?" -> ["hello,", "world!", "how", "are", "you?"] -> sort -> unique -> reverse -> join
    auto result = exec(script);
    ASSERT_TRUE(result.is_string());
    
    // The result should contain all the words in processed form
    std::string result_str = result.get<std::string>();
    EXPECT_TRUE(result_str.find("hello") != std::string::npos);
    EXPECT_TRUE(result_str.find("world") != std::string::npos);
}

TEST_F(StringArrayOperationsTest, ArrayProcessingPipeline) {
    json script = json::array({
        "let",
        json::array({
            json::array({"data", json::object({{"array", json::array({3, 1, 4, 1, 5, 9, 2, 6, 5, 3})}})})
        }),
        json::array({
            "unique",
            json::array({
                "sort",
                json::array({"$", "/data"})
            })
        })
    });
    
    // Should: sort -> unique (firsts mode)
    // [3,1,4,1,5,9,2,6,5,3] -> [1,1,2,3,3,4,5,5,6,9] -> [1,2,3,4,5,6,9]
    json expected = json::object({{"array", json::array({1, 2, 3, 4, 5, 6, 9})}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitJoinRoundTrip) {
    json script = json::array({
        "let",
        json::array({
            json::array({"text", "apple,banana,cherry"})
        }),
        json::array({
            "join",
            json::array({
                "split",
                json::array({"$", "/text"}),
                ","
            }),
            ","
        })
    });
    
    // Should be identical to the original
    EXPECT_EQ(exec(script), "apple,banana,cherry");
}

// Additional sort tests for enhanced API
TEST_F(StringArrayOperationsTest, SortOperatorNestedField) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({
            json::object({{"user", json::object({{"name", "charlie"}})}, {"score", 90}}),
            json::object({{"user", json::object({{"name", "alice"}})}, {"score", 85}}),
            json::object({{"user", json::object({{"name", "bob"}})}, {"score", 95}})
        })}}),
        "/user/name"
    });
    auto result = exec(script);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    auto arr = result["array"];
    ASSERT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0]["user"]["name"], "alice");
    EXPECT_EQ(arr[1]["user"]["name"], "bob");
    EXPECT_EQ(arr[2]["user"]["name"], "charlie");
}

TEST_F(StringArrayOperationsTest, SortOperatorMissingField) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({
            json::object({{"name", "alice"}, {"age", 30}}),
            json::object({{"name", "bob"}}),  // missing age
            json::object({{"name", "charlie"}, {"age", 25}})
        })}}),
        "/age"
    });
    auto result = exec(script);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    auto arr = result["array"];
    ASSERT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), 3);
    // Objects with missing fields should sort first (null < numbers)
    EXPECT_EQ(arr[0]["name"], "bob");
    EXPECT_EQ(arr[1]["name"], "charlie");
    EXPECT_EQ(arr[2]["name"], "alice");
}

TEST_F(StringArrayOperationsTest, SortOperatorComplexMultiField) {
    json script = json::array({"sort", 
        json::object({{"array", json::array({
            json::object({{"dept", "eng"}, {"level", 3}, {"salary", 90000}}),
            json::object({{"dept", "hr"}, {"level", 2}, {"salary", 70000}}),
            json::object({{"dept", "eng"}, {"level", 2}, {"salary", 80000}}),
            json::object({{"dept", "hr"}, {"level", 3}, {"salary", 85000}})
        })}}),
        "/dept",
        json::array({"/level", "desc"}),
        "/salary"
    });
    auto result = exec(script);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    auto arr = result["array"];
    ASSERT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), 4);
    // Sort by: dept asc, level desc, salary asc
    EXPECT_EQ(arr[0]["dept"], "eng");
    EXPECT_EQ(arr[0]["level"], 3);
    EXPECT_EQ(arr[1]["dept"], "eng");
    EXPECT_EQ(arr[1]["level"], 2);
    EXPECT_EQ(arr[2]["dept"], "hr");
    EXPECT_EQ(arr[2]["level"], 3);
    EXPECT_EQ(arr[3]["dept"], "hr");
    EXPECT_EQ(arr[3]["level"], 2);
}