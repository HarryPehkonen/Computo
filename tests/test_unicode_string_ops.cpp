#include <computo.hpp>
#include <gtest/gtest.h>
#include <iostream>

using json = nlohmann::json;

class UnicodeStringOpsTest : public ::testing::Test {
protected:
    void SetUp() override { 
        input_data = json{{"test", "value"}}; 
    }

    auto execute_script(const std::string& script_json) -> json {
        auto script = json::parse(script_json);
        return computo::execute(script, {input_data});
    }

    static auto execute_script(const std::string& script_json, const json& input) -> json {
        auto script = json::parse(script_json);
        return computo::execute(script, {input});
    }

    // Helper to print results for debugging
    void debug_result(const std::string& test_name, const json& result) {
        std::cout << "=== " << test_name << " ===" << std::endl;
        std::cout << "Result: " << result.dump() << std::endl;
        if (result.is_object() && result.contains("array")) {
            std::cout << "Array contents: ";
            for (const auto& item : result["array"]) {
                std::cout << "\"" << item.get<std::string>() << "\" ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    json input_data;
};

// === Unicode Split Tests ===

TEST_F(UnicodeStringOpsTest, SplitUnicodeBasicDelimiter) {
    // Test splitting Unicode string with ASCII delimiter
    auto result = execute_script(R"(["split", "café,naïve,résumé", ","])");
    debug_result("SplitUnicodeBasicDelimiter", result);
    
    auto expected = json::parse(R"({"array": ["café", "naïve", "résumé"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(UnicodeStringOpsTest, SplitUnicodeDelimiter) {
    // Test splitting with Unicode delimiter
    auto result = execute_script(R"(["split", "hello→world→test", "→"])");
    debug_result("SplitUnicodeDelimiter", result);
    
    auto expected = json::parse(R"({"array": ["hello", "world", "test"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(UnicodeStringOpsTest, SplitEmptyDelimiterMultiByte) {
    // FIXED: Now correctly splits Unicode characters, not bytes
    auto result = execute_script(R"(["split", "café", ""])");
    debug_result("SplitEmptyDelimiterMultiByte", result);
    
    // Should now correctly produce: ["c", "a", "f", "é"]
    auto expected = json::parse(R"({"array": ["c", "a", "f", "é"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(UnicodeStringOpsTest, SplitEmptyDelimiterEmoji) {
    // FIXED: Now correctly handles emoji (4-byte UTF-8 sequences)
    auto result = execute_script(R"(["split", "🚀🌟", ""])");
    debug_result("SplitEmptyDelimiterEmoji", result);
    
    // Should now correctly produce: ["🚀", "🌟"]
    auto expected = json::parse(R"({"array": ["🚀", "🌟"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(UnicodeStringOpsTest, SplitCJKCharacters) {
    // Test with Chinese/Japanese characters
    auto result = execute_script(R"(["split", "你好,世界", ","])");
    debug_result("SplitCJKCharacters", result);
    
    auto expected = json::parse(R"({"array": ["你好", "世界"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(UnicodeStringOpsTest, SplitMixedScript) {
    // Mixed scripts and emoji
    auto result = execute_script(R"(["split", "Hello世界🌍Мир", "世"])");
    debug_result("SplitMixedScript", result);
    
    auto expected = json::parse(R"({"array": ["Hello", "界🌍Мир"]})");
    EXPECT_EQ(result, expected);
}

// === Unicode Join Tests ===

TEST_F(UnicodeStringOpsTest, JoinUnicodeStrings) {
    auto result = execute_script(R"(["join", {"array": ["café", "naïve", "résumé"]}, " • "])");
    debug_result("JoinUnicodeStrings", result);
    
    EXPECT_EQ(result, json("café • naïve • résumé"));
}

TEST_F(UnicodeStringOpsTest, JoinWithUnicodeDelimiter) {
    auto result = execute_script(R"(["join", {"array": ["hello", "world"]}, " → "])");
    debug_result("JoinWithUnicodeDelimiter", result);
    
    EXPECT_EQ(result, json("hello → world"));
}

TEST_F(UnicodeStringOpsTest, JoinEmoji) {
    auto result = execute_script(R"(["join", {"array": ["🚀", "🌟", "⭐"]}, ""])");
    debug_result("JoinEmoji", result);
    
    EXPECT_EQ(result, json("🚀🌟⭐"));
}

// === Unicode Trim Tests ===

TEST_F(UnicodeStringOpsTest, TrimASCIIWhitespace) {
    // This should work fine
    auto result = execute_script(R"(["trim", "   café   "])");
    debug_result("TrimASCIIWhitespace", result);
    
    EXPECT_EQ(result, json("café"));
}

TEST_F(UnicodeStringOpsTest, TrimUnicodeWhitespace) {
    // Test with Unicode whitespace characters
    // U+00A0 (Non-breaking space), U+2003 (Em space), U+2009 (Thin space)
    auto result = execute_script(R"(["trim", " café "])");
    debug_result("TrimUnicodeWhitespace", result);
    
    // Current implementation likely won't trim Unicode whitespace
    std::cout << "WARNING: Unicode whitespace may not be trimmed!" << std::endl;
}

TEST_F(UnicodeStringOpsTest, TrimZeroWidthSpaces) {
    // U+200B (Zero width space), U+FEFF (Byte order mark/zero width no-break space)
    std::string input_with_zwsp = "\u200B\uFEFFcafé\u200B\uFEFF";
    auto script = R"(["trim", ")" + input_with_zwsp + R"("])";
    
    try {
        auto result = execute_script(script);
        debug_result("TrimZeroWidthSpaces", result);
        std::cout << "Zero-width spaces likely not trimmed by current implementation" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error with zero-width space test: " << e.what() << std::endl;
    }
}

// === Unicode Case Conversion Tests ===

TEST_F(UnicodeStringOpsTest, UpperASCII) {
    auto result = execute_script(R"(["upper", "hello"])");
    debug_result("UpperASCII", result);
    
    EXPECT_EQ(result, json("HELLO"));
}

TEST_F(UnicodeStringOpsTest, UpperAccentedCharacters) {
    auto result = execute_script(R"(["upper", "café"])");
    debug_result("UpperAccentedCharacters", result);
    
    // FIXED: Now correctly converts é to É
    EXPECT_EQ(result, json("CAFÉ"));
}

TEST_F(UnicodeStringOpsTest, LowerAccentedCharacters) {
    auto result = execute_script(R"(["lower", "CAFÉ"])");
    debug_result("LowerAccentedCharacters", result);
    
    // Current implementation will likely NOT convert É to é
    std::cout << "Expected: café, but É likely won't convert to é" << std::endl;
}

TEST_F(UnicodeStringOpsTest, UpperCyrillic) {
    auto result = execute_script(R"(["upper", "привет"])");
    debug_result("UpperCyrillic", result);
    
    std::cout << "Expected: ПРИВЕТ, but Cyrillic case conversion likely won't work" << std::endl;
}

TEST_F(UnicodeStringOpsTest, UpperGreek) {
    auto result = execute_script(R"(["upper", "αβγ"])");
    debug_result("UpperGreek", result);
    
    std::cout << "Expected: ΑΒΓ, but Greek case conversion likely won't work" << std::endl;
}

TEST_F(UnicodeStringOpsTest, UpperGerman) {
    auto result = execute_script(R"(["upper", "straße"])");
    debug_result("UpperGerman", result);
    
    std::cout << "Expected: STRASSE (ß → SS), but this won't work with ASCII-only conversion" << std::endl;
}

// === Unicode String Concatenation Tests ===

TEST_F(UnicodeStringOpsTest, StrConcatUnicode) {
    auto result = execute_script(R"(["strConcat", "Hello ", "世界", " 🌍"])");
    debug_result("StrConcatUnicode", result);
    
    EXPECT_EQ(result, json("Hello 世界 🌍"));
}

TEST_F(UnicodeStringOpsTest, StrConcatMixedTypes) {
    auto result = execute_script(R"(["strConcat", "Score: ", 42, " 🏆"])");
    debug_result("StrConcatMixedTypes", result);
    
    EXPECT_EQ(result, json("Score: 42 🏆"));
}

// === Roundtrip Tests ===

TEST_F(UnicodeStringOpsTest, SplitJoinRoundtripUnicode) {
    auto result = execute_script(R"(["join", ["split", "café→naïve→résumé", "→"], "→"])");
    debug_result("SplitJoinRoundtripUnicode", result);
    
    EXPECT_EQ(result, json("café→naïve→résumé"));
}

TEST_F(UnicodeStringOpsTest, SplitJoinRoundtripEmoji) {
    auto result = execute_script(R"(["join", ["split", "🚀,🌟,⭐", ","], ","])");
    debug_result("SplitJoinRoundtripEmoji", result);
    
    EXPECT_EQ(result, json("🚀,🌟,⭐"));
}

// === Edge Cases and Error Conditions ===

TEST_F(UnicodeStringOpsTest, InvalidUTF8Handling) {
    // Test how the system handles invalid UTF-8 sequences
    // This is tricky to test directly, so we'll just ensure no crashes
    std::cout << "Testing invalid UTF-8 handling..." << std::endl;
    
    // Most invalid UTF-8 will be caught by JSON parsing, so this is limited
    SUCCEED(); // Just ensure we can run this test without crashing
}

TEST_F(UnicodeStringOpsTest, EmptyUnicodeStrings) {
    auto result = execute_script(R"(["strConcat", "", "café", ""])");
    debug_result("EmptyUnicodeStrings", result);
    
    EXPECT_EQ(result, json("café"));
}

TEST_F(UnicodeStringOpsTest, UnicodeNormalization) {
    // Test with composed vs decomposed characters
    // "é" can be U+00E9 (composed) or U+0065 U+0301 (e + combining acute)
    std::cout << "Note: Unicode normalization not tested - would require composed/decomposed character pairs" << std::endl;
    SUCCEED();
}

// === Performance and Length Tests ===

TEST_F(UnicodeStringOpsTest, UnicodeLengthAwareness) {
    // While we can't test string length directly, we can observe split behavior
    auto result = execute_script(R"(["split", "🚀", ""])");
    debug_result("UnicodeLengthAwareness", result);
    
    std::cout << "Single emoji character split result - should be 1 element, but likely more due to byte-level splitting" << std::endl;
}

// === Documentation Helper Tests ===

TEST_F(UnicodeStringOpsTest, DocumentCurrentBehavior) {
    std::cout << "\n=== UNICODE SUPPORT ANALYSIS ===" << std::endl;
    std::cout << "Running comprehensive tests to document current Unicode behavior..." << std::endl;
    std::cout << "Each test will show expected vs actual behavior." << std::endl;
    std::cout << "Check the debug output above for detailed analysis." << std::endl;
    
    SUCCEED(); // This test always passes, it's just for documentation
}
