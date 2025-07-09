#include <gtest/gtest.h>
#include <computo.hpp>
using json = nlohmann::json;

class ZipObjectCreationTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(ZipObjectCreationTest, BasicZipToObject) {
    // Using reduce with zip to create object from separate arrays
    json script = json::array({
        "reduce", 
        json::array({"zip", json::object({{"array", json::array({"a", "b", "c"})}}), 
                           json::object({{"array", json::array({2, 4, 6})}})}),
        json::array({"lambda", json::array({"x"}), 
            json::array({"merge", 
                json::array({"get", json::array({"$", "/x"}), "/0"}),
                json::array({"obj", json::array({
                    json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/0"}),
                    json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/1"})
                })})
            })
        }),
        json::object()
    });
    
    json expected = json::object({{"a", 2}, {"b", 4}, {"c", 6}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ZipObjectCreationTest, ZipToObjectWithLetBindings) {
    // More readable version using let bindings
    json script = json::array({
        "let", 
        json::array({
            json::array({"keys", json::object({{"array", json::array({"x", "y", "z"})}})}),
            json::array({"values", json::object({{"array", json::array({10, 20, 30})}})})
        }),
        json::array({
            "reduce",
            json::array({"zip", json::array({"$", "/keys"}), json::array({"$", "/values"})}),
            json::array({"lambda", json::array({"x"}),
                json::array({"let", 
                    json::array({
                        json::array({"acc", json::array({"get", json::array({"$", "/x"}), "/0"})}),
                        json::array({"pair", json::array({"get", json::array({"$", "/x"}), "/1"})})
                    }),
                    json::array({"merge", 
                        json::array({"$", "/acc"}),
                        json::array({"obj", json::array({
                            json::array({"get", json::array({"$", "/pair"}), "/0"}),
                            json::array({"get", json::array({"$", "/pair"}), "/1"})
                        })})
                    })
                })
            }),
            json::object()
        })
    });
    
    json expected = json::object({{"x", 10}, {"y", 20}, {"z", 30}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ZipObjectCreationTest, ZipToObjectWithVariablePointerSyntax) {
    // Testing the simpler $ pointer syntax for array access
    json script = json::array({
        "let",
        json::array({
            json::array({"pairs", json::array({"zip", 
                json::object({{"array", json::array({"foo", "bar"})}}),
                json::object({{"array", json::array({"hello", "world"})}})
            })})
        }),
        json::array({
            "reduce",
            json::array({"$", "/pairs"}),
            json::array({"lambda", json::array({"x"}),
                json::array({"let",
                    json::array({
                        json::array({"acc", json::array({"get", json::array({"$", "/x"}), "/0"})}),
                        json::array({"pair", json::array({"get", json::array({"$", "/x"}), "/1"})})
                    }),
                    json::array({"merge",
                        json::array({"$", "/acc"}),
                        json::array({"obj", json::array({
                            json::array({"$", "/pair/0"}),
                            json::array({"$", "/pair/1"})
                        })})
                    })
                })
            }),
            json::object()
        })
    });
    
    json expected = json::object({{"foo", "hello"}, {"bar", "world"}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ZipObjectCreationTest, ZipToObjectEmptyArrays) {
    // Test with empty arrays
    json script = json::array({
        "reduce",
        json::array({"zip", 
            json::object({{"array", json::array()}}),
            json::object({{"array", json::array()}})
        }),
        json::array({"lambda", json::array({"x"}),
            json::array({"merge",
                json::array({"get", json::array({"$", "/x"}), "/0"}),
                json::array({"obj", json::array({
                    json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/0"}),
                    json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/1"})
                })})
            })
        }),
        json::object()
    });
    
    json expected = json::object();
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ZipObjectCreationTest, ZipToObjectUnequalLengths) {
    // Test with unequal length arrays - should only process up to shorter length
    json script = json::array({
        "reduce",
        json::array({"zip",
            json::object({{"array", json::array({"a", "b", "c", "d", "e"})}}),
            json::object({{"array", json::array({1, 2, 3})}})
        }),
        json::array({"lambda", json::array({"x"}),
            json::array({"merge",
                json::array({"get", json::array({"$", "/x"}), "/0"}),
                json::array({"obj", json::array({
                    json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/0"}),
                    json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/1"})
                })})
            })
        }),
        json::object()
    });
    
    json expected = json::object({{"a", 1}, {"b", 2}, {"c", 3}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ZipObjectCreationTest, ZipToObjectWithComplexValues) {
    // Test with complex values (nested objects and arrays)
    json script = json::array({
        "reduce",
        json::array({"zip",
            json::object({{"array", json::array({"data", "meta"})}}),
            json::object({{"array", json::array({
                json::object({{"array", json::array({1, 2, 3})}}),
                json::object({{"version", "1.0"}, {"author", "test"}})
            })}})
        }),
        json::array({"lambda", json::array({"x"}),
            json::array({"merge",
                json::array({"get", json::array({"$", "/x"}), "/0"}),
                json::array({"obj", json::array({
                    json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/0"}),
                    json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/1"})
                })})
            })
        }),
        json::object()
    });
    
    json expected = json::object({
        {"data", json::object({{"array", json::array({1, 2, 3})}})},
        {"meta", json::object({{"version", "1.0"}, {"author", "test"}})}
    });
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ZipObjectCreationTest, ZipToObjectWithDynamicKeys) {
    // Test where keys are computed dynamically
    json script = json::array({
        "let",
        json::array({
            json::array({"prefix", "key_"}),
            json::array({"suffixes", json::object({{"array", json::array({"a", "b", "c"})}})}),
            json::array({"values", json::object({{"array", json::array({100, 200, 300})}})})
        }),
        json::array({
            "reduce",
            json::array({"zip",
                json::array({"map", json::array({"$", "/suffixes"}),
                    json::array({"lambda", json::array({"s"}),
                        json::array({"strConcat", json::array({"$", "/prefix"}), json::array({"$", "/s"})})
                    })
                }),
                json::array({"$", "/values"})
            }),
            json::array({"lambda", json::array({"x"}),
                json::array({"merge",
                    json::array({"get", json::array({"$", "/x"}), "/0"}),
                    json::array({"obj", json::array({
                        json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/0"}),
                        json::array({"get", json::array({"get", json::array({"$", "/x"}), "/1"}), "/1"})
                    })})
                })
            }),
            json::object()
        })
    });
    
    json expected = json::object({{"key_a", 100}, {"key_b", 200}, {"key_c", 300}});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ZipObjectCreationTest, DirectArrayAccessInReduce) {
    // Test using direct array access within reduce lambda
    json script = json::array({
        "let",
        json::array({
            json::array({"zipped", json::array({"zip",
                json::object({{"array", json::array({"one", "two"})}}),
                json::object({{"array", json::array({1.0, 2.0})}})
            })})
        }),
        json::array({
            "reduce",
            json::array({"$", "/zipped"}),
            json::array({"lambda", json::array({"acc_pair"}),
                json::array({"merge",
                    json::array({"get", json::array({"$", "/acc_pair"}), "/0"}),
                    json::array({"obj", json::array({
                        json::array({"get", json::array({"get", json::array({"$", "/acc_pair"}), "/1"}), "/0"}),
                        json::array({"get", json::array({"get", json::array({"$", "/acc_pair"}), "/1"}), "/1"})
                    })})
                })
            }),
            json::object()
        })
    });
    
    json expected = json::object({{"one", 1.0}, {"two", 2.0}});
    EXPECT_EQ(exec(script), expected);
} 