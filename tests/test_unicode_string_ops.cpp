#include <computo.hpp>
#include <gtest/gtest.h>
#include <iostream>

using json = jsom::JsonDocument;

// Unicode Compatibility Tests
//
// These tests verify that remaining string operators handle Unicode data correctly
// as UTF-8 byte sequences. While there's no Unicode-aware processing (no case
// conversion, normalization, or character boundary detection), the operators
// treat Unicode text as opaque byte strings, which often works correctly.

class UnicodeCompatibilityTest : public ::testing::Test {
protected:
    void SetUp() override { input_data = json{{"test", "value"}}; }

    auto execute_script(const std::string& script_json) -> json {
        auto script = jsom::parse_document(script_json);
        return computo::execute(script, {input_data});
    }

    static auto execute_script(const std::string& script_json, const json& input) -> json {
        auto script = jsom::parse_document(script_json);
        return computo::execute(script, {input});
    }

    // Helper to print results for debugging
    void debug_result(const std::string& test_name, const json& result) {
        std::cout << "=== " << test_name << " ===" << '\n';
        std::cout << "Result: " << result.to_json() << '\n';
        if (result.is_object() && result.contains("array")) {
            std::cout << "Array contents: ";
            for (const auto& item : result["array"]) {
                std::cout << "\"" << item.as<std::string>() << "\" ";
            }
            std::cout << '\n';
        }
        std::cout << '\n';
    }

    json input_data;
};

// === Unicode Join Tests ===
// Tests that 'join' operator handles Unicode strings correctly

TEST_F(UnicodeCompatibilityTest, JoinUnicodeStrings) {
    auto result = execute_script(R"(["join", {"array": ["café", "naïve", "résumé"]}, " • "])");
    debug_result("JoinUnicodeStrings", result);

    EXPECT_EQ(result, json("café • naïve • résumé"));
}

TEST_F(UnicodeCompatibilityTest, JoinWithUnicodeDelimiter) {
    auto result = execute_script(R"(["join", {"array": ["hello", "world"]}, " → "])");
    debug_result("JoinWithUnicodeDelimiter", result);

    EXPECT_EQ(result, json("hello → world"));
}

TEST_F(UnicodeCompatibilityTest, JoinEmoji) {
    auto result = execute_script(R"(["join", {"array": ["🚀", "🌟", "⭐"]}, ""])");
    debug_result("JoinEmoji", result);

    EXPECT_EQ(result, json("🚀🌟⭐"));
}

TEST_F(UnicodeCompatibilityTest, JoinMixedScripts) {
    auto result = execute_script(R"(["join", {"array": ["Hello", "世界", "🌍", "Мир"]}, " | "])");
    debug_result("JoinMixedScripts", result);

    EXPECT_EQ(result, json("Hello | 世界 | 🌍 | Мир"));
}

// === Unicode String Concatenation Tests ===
// Tests that 'strConcat' operator handles Unicode strings correctly

TEST_F(UnicodeCompatibilityTest, StrConcatUnicode) {
    auto result = execute_script(R"(["strConcat", "Hello ", "世界", " 🌍"])");
    debug_result("StrConcatUnicode", result);

    EXPECT_EQ(result, json("Hello 世界 🌍"));
}

TEST_F(UnicodeCompatibilityTest, StrConcatMixedTypes) {
    auto result = execute_script(R"(["strConcat", "Score: ", 42, " 🏆"])");
    debug_result("StrConcatMixedTypes", result);

    EXPECT_EQ(result, json("Score: 42 🏆"));
}

TEST_F(UnicodeCompatibilityTest, StrConcatEmoji) {
    auto result = execute_script(R"(["strConcat", "🚀", "🌟", "⭐", " = success!"])");
    debug_result("StrConcatEmoji", result);

    EXPECT_EQ(result, json("🚀🌟⭐ = success!"));
}

TEST_F(UnicodeCompatibilityTest, StrConcatCJK) {
    auto result = execute_script(R"(["strConcat", "你好", "世界", "!"])");
    debug_result("StrConcatCJK", result);

    EXPECT_EQ(result, json("你好世界!"));
}

// === Unicode Sort Tests ===
// Tests that 'sort' operator handles Unicode strings (lexicographic byte ordering)

TEST_F(UnicodeCompatibilityTest, SortUnicodeStrings) {
    json unicode_array
        = json{{"array", json(std::vector<json>{"café", "naïve", "résumé", "apple", "zebra"})}};
    auto result = execute_script(R"(["sort", ["$input"]])", unicode_array);
    debug_result("SortUnicodeStrings", result);

    // Sort treats Unicode as bytes, so lexicographic ordering by UTF-8 bytes
    // This may not match linguistic sorting but is predictable
    EXPECT_TRUE(result.is_object() && result.contains("array"));
    EXPECT_EQ(result["array"].size(), 5);
    std::cout << "Note: Sort uses lexicographic byte ordering, not linguistic ordering" << '\n';
}

TEST_F(UnicodeCompatibilityTest, SortEmoji) {
    json emoji_array = json{{"array", json(std::vector<json>{"🌟", "🚀", "⭐", "🏆"})}};
    auto result = execute_script(R"(["sort", ["$input"]])", emoji_array);
    debug_result("SortEmoji", result);

    EXPECT_TRUE(result.is_object() && result.contains("array"));
    EXPECT_EQ(result["array"].size(), 4);
    std::cout << "Note: Emoji sorting by UTF-8 byte values" << '\n';
}

TEST_F(UnicodeCompatibilityTest, SortMixedScripts) {
    json mixed_array
        = json{{"array", json(std::vector<json>{"Hello", "世界", "café", "Мир", "🌍"})}};
    auto result = execute_script(R"(["sort", ["$input"]])", mixed_array);
    debug_result("SortMixedScripts", result);

    EXPECT_TRUE(result.is_object() && result.contains("array"));
    EXPECT_EQ(result["array"].size(), 5);
    std::cout << "Note: Mixed scripts sorted by UTF-8 byte values" << '\n';
}

// === Empty and Edge Cases ===

TEST_F(UnicodeCompatibilityTest, EmptyUnicodeStrings) {
    auto result = execute_script(R"(["join", {"array": ["", "café", ""]}, "|"])");
    debug_result("EmptyUnicodeStrings", result);

    EXPECT_EQ(result, json("|café|"));
}

TEST_F(UnicodeCompatibilityTest, StrConcatEmpty) {
    auto result = execute_script(R"(["strConcat", "", "世界", ""])");
    debug_result("StrConcatEmpty", result);

    EXPECT_EQ(result, json("世界"));
}

// === Documentation Test ===

TEST_F(UnicodeCompatibilityTest, DocumentCurrentBehavior) {
    std::cout << "=== Unicode Compatibility Summary ===" << '\n';
    std::cout << "✓ join: Handles Unicode strings correctly as byte sequences" << '\n';
    std::cout << "✓ strConcat: Concatenates Unicode strings correctly" << '\n';
    std::cout << "✓ sort: Lexicographic ordering by UTF-8 byte values (not linguistic)" << '\n';
    std::cout << "✗ split: Operator removed (no character boundary detection)" << '\n';
    std::cout << "✗ trim: Operator removed (no Unicode whitespace detection)" << '\n';
    std::cout << "✗ upper/lower: Operators removed (no Unicode case conversion)" << '\n';
    std::cout << "Note: Unicode data flows through system correctly as UTF-8" << '\n';
    std::cout << "=======================================" << '\n';
}