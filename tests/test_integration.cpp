#include <gtest/gtest.h>
#include <computo.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

class IntegrationTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(IntegrationTest, SimpleDataTransformation) {
    json input_data = json::object({
        {"users", json::array({
            json::object({{"id", 1}, {"name", "Alice"}, {"age", 25}}),
            json::object({{"id", 2}, {"name", "Bob"}, {"age", 30}})
        })}
    });
    
    json script = json::array({
        "let",
        json::array({json::array({"data", json::array({"$input"})})}),
        json::array({
            "let",
            json::array({json::array({"users", json::array({"get", json::array({"$", "/data"}), "/users"})})}),
            json::array({
                "count",
                json::array({"$", "/users"})
            })
        })
    });
    
    auto result = exec(script, input_data);
    EXPECT_EQ(result, 2);
}

TEST_F(IntegrationTest, MathematicalComputation) {
    json script = json::array({
        "let",
        json::array({
            json::array({"numbers", json::object({{"array", json::array({1, 2, 3, 4, 5})}})})
        }),
        json::array({
            "let",
            json::array({json::array({"squared", json::array({
                "map",
                json::array({"$", "/numbers"}),
                json::array({"lambda", json::array({"x"}), json::array({"*", json::array({"$", "/x"}), json::array({"$", "/x"})})})
            })})}),
            json::array({
                "reduce",
                json::array({"$", "/squared"}),
                json::array({"lambda", json::array({"args"}), json::array({
                    "+",
                    json::array({"get", json::array({"$", "/args"}), "/0"}),
                    json::array({"get", json::array({"$", "/args"}), "/1"})
                })}),
                0
            })
        })
    });
    
    auto result = exec(script);
    EXPECT_EQ(result, 55); // 1+4+9+16+25 = 55
}

TEST_F(IntegrationTest, StringProcessing) {
    json input_data = json::object({
        {"texts", json::array({"Hello", "World"})}
    });
    
    json script = json::array({
        "let",
        json::array({json::array({"data", json::array({"$input"})})}),
        json::array({
            "let",
            json::array({json::array({"texts", json::array({"get", json::array({"$", "/data"}), "/texts"})})}),
            json::array({
                "strConcat",
                json::array({"get", json::array({"$", "/texts"}), "/0"}),
                " ",
                json::array({"get", json::array({"$", "/texts"}), "/1"})
            })
        })
    });
    
    auto result = exec(script, input_data);
    EXPECT_EQ(result, "Hello World");
} 