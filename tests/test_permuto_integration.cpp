#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class PermutoIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test permuto.apply basic functionality
TEST_F(PermutoIntegrationTest, PermutoApplyBasic) {
    json template_json = json{{"greeting", "${/name}"}};
    json context = json{{"name", "World"}};
    
    // Builder Pattern makes template application crystal clear!
    auto script = CB::permuto_apply(template_json, context);
    
    json expected = json{{"greeting", "World"}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test permuto.apply with expressions
TEST_F(PermutoIntegrationTest, PermutoApplyWithExpressions) {
    json template_json = json{{"user", "${/user/name}"}, {"count", "${/items/length}"}};
    
    // Build context using Computo operators - Builder Pattern makes this elegant!
    auto script = CB::permuto_apply(
        template_json,
        CB::obj()
            .add_field("user", CB::obj().add_field("name", "Alice"))
            .add_field("items", CB::obj().add_field("length", 5))
    );
    
    json expected = json{{"user", "Alice"}, {"count", 5}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test permuto.apply with input data
TEST_F(PermutoIntegrationTest, PermutoApplyWithInput) {
    json test_input = json{{"user", json{{"id", 123}, {"name", "Bob"}}}};
    json template_json = json{{"user_id", "${/user/id}"}, {"greeting", "Hello ${/user/name}!"}};
    
    // Template application with input data
    auto script = CB::permuto_apply(template_json, CB::input());
    
    // Create permuto options with interpolation enabled
    permuto::Options opts;
    opts.enable_interpolation = true;
    
    json expected = json{{"user_id", 123}, {"greeting", "Hello Bob!"}};
    EXPECT_EQ(computo::execute(script, test_input, opts), expected);
}

// Test permuto.apply with variables from let
TEST_F(PermutoIntegrationTest, PermutoApplyWithLet) {
    json template_json = json{{"message", "${/msg}"}, {"value", "${/val}"}};
    
    // Variable binding + template application - Builder Pattern shines here!
    auto script = CB::let(
        {{"data", CB::obj()
            .add_field("msg", "hello")
            .add_field("val", 42)
        }},
        CB::permuto_apply(template_json, CB::var("data"))
    );
    
    json expected = json{{"message", "hello"}, {"value", 42}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test permuto.apply with complex template
TEST_F(PermutoIntegrationTest, PermutoApplyComplexTemplate) {
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
    
    auto script = CB::permuto_apply(template_json, CB::input());
    
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
TEST_F(PermutoIntegrationTest, PermutoApplyConditional) {
    json test_input = json{{"user", json{{"active", true}, {"name", "Charlie"}}}};
    
    // Conditional template selection - Builder Pattern makes the flow obvious
    auto script = CB::if_then_else(
        CB::get(CB::input(), "/user/active"),
        // Active user template
        CB::permuto_apply(
            json{{"status", "active"}, {"greeting", "Welcome ${/user/name}!"}},
            CB::input()
        ),
        // Inactive user template  
        CB::permuto_apply(
            json{{"status", "inactive"}, {"message", "Account suspended"}},
            CB::input()
        )
    );
    
    permuto::Options opts;
    opts.enable_interpolation = true;
    
    json expected = json{{"status", "active"}, {"greeting", "Welcome Charlie!"}};
    EXPECT_EQ(computo::execute(script, test_input, opts), expected);
}

// Test permuto.apply with array data
TEST_F(PermutoIntegrationTest, PermutoApplyWithArrayData) {
    json template_json = json{
        {"users", json::array({
            "${/users/0/name}",
            "${/users/1/name}",
            "${/users/2/name}"
        })},
        {"count", "${/length}"}
    };
    
    json test_input = json{
        {"users", json::array({
            json{{"name", "Alice"}},
            json{{"name", "Bob"}},
            json{{"name", "Charlie"}}
        })},
        {"length", 3}
    };
    
    auto script = CB::permuto_apply(template_json, CB::input());
    
    json expected = json{
        {"users", json::array({"Alice", "Bob", "Charlie"})},
        {"count", 3}
    };
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test permuto.apply in functional pipeline
TEST_F(PermutoIntegrationTest, PermutoApplyInPipeline) {
    // Create data, transform it, then apply template - showcases Computo+Permuto power!
    auto script = CB::let(
        {{"processed_data", CB::map(
            CB::array({
                json{{"id", 1}, {"name", "Alice"}},
                json{{"id", 2}, {"name", "Bob"}}
            }),
            CB::lambda("user", 
                CB::obj()
                    .add_field("user_id", CB::get(CB::var("user"), "/id"))
                    .add_field("display_name", CB::get(CB::var("user"), "/name"))
            )
        )}},
        CB::permuto_apply(
            json{
                {"title", "User Report"},
                {"users", "${/processed_data}"},
                {"generated_at", "${/timestamp}"}
            },
            CB::obj()
                .add_field("processed_data", CB::var("processed_data"))
                .add_field("timestamp", "2024-01-01T00:00:00Z")
        )
    );
    
    json expected = json{
        {"title", "User Report"},
        {"users", json::array({
            json{{"user_id", 1}, {"display_name", "Alice"}},
            json{{"user_id", 2}, {"display_name", "Bob"}}
        })},
        {"generated_at", "2024-01-01T00:00:00Z"}
    };
    
    EXPECT_EQ(computo::execute(script, input_data), expected);
}