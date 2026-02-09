#include "json_colorizer.hpp"
#include <gtest/gtest.h>
#include <regex>
#include <string>

using computo::ColorMode;
using computo::JsonColorTheme;
using computo::JsonColorizer;

// Helper: strip all ANSI escape codes from a string
static auto strip_ansi(const std::string& input) -> std::string {
    static const std::regex ansi_re("\033\\[[0-9;]*m");
    return std::regex_replace(input, ansi_re, "");
}

// Helper: check if a substring is wrapped with expected ANSI code
static auto contains_colored(const std::string& output, const char* ansi_code, const std::string& text) -> bool {
    std::string pattern = std::string(ansi_code) + text + "\033[0m";
    return output.find(pattern) != std::string::npos;
}

class JsonColorizerTest : public ::testing::Test {
protected:
    JsonColorTheme theme = JsonColorTheme::default_theme();
};

// --- Token coloring correctness ---

TEST_F(JsonColorizerTest, ObjectKeysAreCyan) {
    auto result = JsonColorizer::colorize(R"({"name":"Alice"})", theme);
    EXPECT_TRUE(contains_colored(result, theme.key, "\"name\""));
}

TEST_F(JsonColorizerTest, StringValuesAreGreen) {
    auto result = JsonColorizer::colorize(R"({"name":"Alice"})", theme);
    EXPECT_TRUE(contains_colored(result, theme.string, "\"Alice\""));
}

TEST_F(JsonColorizerTest, NumbersAreBrightWhite) {
    auto result = JsonColorizer::colorize(R"({"age":42})", theme);
    EXPECT_TRUE(contains_colored(result, theme.number, "42"));
}

TEST_F(JsonColorizerTest, NegativeNumbers) {
    auto result = JsonColorizer::colorize(R"([-3.14])", theme);
    EXPECT_TRUE(contains_colored(result, theme.number, "-3.14"));
}

TEST_F(JsonColorizerTest, ScientificNotation) {
    auto result = JsonColorizer::colorize(R"([1.5e10])", theme);
    EXPECT_TRUE(contains_colored(result, theme.number, "1.5e10"));
}

TEST_F(JsonColorizerTest, BooleansAreYellow) {
    auto result = JsonColorizer::colorize(R"({"a":true,"b":false})", theme);
    EXPECT_TRUE(contains_colored(result, theme.boolean, "true"));
    EXPECT_TRUE(contains_colored(result, theme.boolean, "false"));
}

TEST_F(JsonColorizerTest, NullIsDim) {
    auto result = JsonColorizer::colorize(R"({"x":null})", theme);
    EXPECT_TRUE(contains_colored(result, theme.null, "null"));
}

TEST_F(JsonColorizerTest, StructuralCharsAreDim) {
    auto result = JsonColorizer::colorize(R"({"a":1})", theme);
    EXPECT_TRUE(contains_colored(result, theme.structural, "{"));
    EXPECT_TRUE(contains_colored(result, theme.structural, "}"));
    EXPECT_TRUE(contains_colored(result, theme.structural, ":"));
}

TEST_F(JsonColorizerTest, BracketsAreDim) {
    auto result = JsonColorizer::colorize(R"([1,2])", theme);
    EXPECT_TRUE(contains_colored(result, theme.structural, "["));
    EXPECT_TRUE(contains_colored(result, theme.structural, "]"));
    EXPECT_TRUE(contains_colored(result, theme.structural, ","));
}

// --- Nested structures ---

TEST_F(JsonColorizerTest, NestedObjectInArray) {
    auto result = JsonColorizer::colorize(R"([{"key":"val"}])", theme);
    EXPECT_TRUE(contains_colored(result, theme.key, "\"key\""));
    EXPECT_TRUE(contains_colored(result, theme.string, "\"val\""));
}

TEST_F(JsonColorizerTest, ArrayInObject) {
    auto result = JsonColorizer::colorize(R"({"nums":[1,2,3]})", theme);
    EXPECT_TRUE(contains_colored(result, theme.key, "\"nums\""));
    EXPECT_TRUE(contains_colored(result, theme.number, "1"));
    EXPECT_TRUE(contains_colored(result, theme.number, "2"));
    EXPECT_TRUE(contains_colored(result, theme.number, "3"));
}

TEST_F(JsonColorizerTest, DeeplyNested) {
    auto result = JsonColorizer::colorize(R"({"a":{"b":{"c":true}}})", theme);
    EXPECT_TRUE(contains_colored(result, theme.key, "\"a\""));
    EXPECT_TRUE(contains_colored(result, theme.key, "\"b\""));
    EXPECT_TRUE(contains_colored(result, theme.key, "\"c\""));
    EXPECT_TRUE(contains_colored(result, theme.boolean, "true"));
}

TEST_F(JsonColorizerTest, StringsInArrayAreValues) {
    auto result = JsonColorizer::colorize(R"(["hello","world"])", theme);
    EXPECT_TRUE(contains_colored(result, theme.string, "\"hello\""));
    EXPECT_TRUE(contains_colored(result, theme.string, "\"world\""));
}

// --- Edge cases ---

TEST_F(JsonColorizerTest, EmptyObject) {
    auto result = JsonColorizer::colorize(R"({})", theme);
    EXPECT_EQ(strip_ansi(result), "{}");
}

TEST_F(JsonColorizerTest, EmptyArray) {
    auto result = JsonColorizer::colorize(R"([])", theme);
    EXPECT_EQ(strip_ansi(result), "[]");
}

TEST_F(JsonColorizerTest, EmptyString) {
    auto result = JsonColorizer::colorize(R"({"k":""})", theme);
    EXPECT_TRUE(contains_colored(result, theme.string, "\"\""));
}

TEST_F(JsonColorizerTest, EscapedQuotesInString) {
    std::string json = R"({"msg":"say \"hello\""})";
    auto result = JsonColorizer::colorize(json, theme);
    EXPECT_EQ(strip_ansi(result), json);
    EXPECT_TRUE(contains_colored(result, theme.key, "\"msg\""));
}

TEST_F(JsonColorizerTest, UnicodeEscapes) {
    std::string json = R"({"emoji":"\u0041\u0042"})";
    auto result = JsonColorizer::colorize(json, theme);
    EXPECT_EQ(strip_ansi(result), json);
}

TEST_F(JsonColorizerTest, BackslashEscapeSequences) {
    std::string json = R"({"path":"C:\\foo\\bar"})";
    auto result = JsonColorizer::colorize(json, theme);
    EXPECT_EQ(strip_ansi(result), json);
}

// --- Roundtrip: stripping ANSI yields original ---

TEST_F(JsonColorizerTest, RoundtripSimple) {
    std::string json = R"({"name":"Alice","age":30,"active":true,"data":null})";
    auto result = JsonColorizer::colorize(json, theme);
    EXPECT_EQ(strip_ansi(result), json);
}

TEST_F(JsonColorizerTest, RoundtripPrettyPrinted) {
    std::string json = "{\n  \"name\": \"Alice\",\n  \"age\": 30\n}";
    auto result = JsonColorizer::colorize(json, theme);
    EXPECT_EQ(strip_ansi(result), json);
}

TEST_F(JsonColorizerTest, RoundtripNestedComplex) {
    std::string json = R"({"users":[{"name":"Bob","scores":[1,2,3],"active":false},{"name":"Eve","scores":[],"active":true}],"count":2,"meta":null})";
    auto result = JsonColorizer::colorize(json, theme);
    EXPECT_EQ(strip_ansi(result), json);
}

TEST_F(JsonColorizerTest, RoundtripTopLevelArray) {
    std::string json = R"([1,"two",true,null,{"k":"v"},[]])";
    auto result = JsonColorizer::colorize(json, theme);
    EXPECT_EQ(strip_ansi(result), json);
}

TEST_F(JsonColorizerTest, RoundtripTopLevelScalar) {
    EXPECT_EQ(strip_ansi(JsonColorizer::colorize("42", theme)), "42");
    EXPECT_EQ(strip_ansi(JsonColorizer::colorize("\"hello\"", theme)), "\"hello\"");
    EXPECT_EQ(strip_ansi(JsonColorizer::colorize("true", theme)), "true");
    EXPECT_EQ(strip_ansi(JsonColorizer::colorize("null", theme)), "null");
}

// --- Multiple keys after commas ---

TEST_F(JsonColorizerTest, MultipleKeysCorrectlyIdentified) {
    auto result = JsonColorizer::colorize(R"({"a":1,"b":2,"c":3})", theme);
    EXPECT_TRUE(contains_colored(result, theme.key, "\"a\""));
    EXPECT_TRUE(contains_colored(result, theme.key, "\"b\""));
    EXPECT_TRUE(contains_colored(result, theme.key, "\"c\""));
}

// --- resolve_color_mode ---

TEST(ColorModeTest, AlwaysReturnsTrue) {
    EXPECT_TRUE(computo::resolve_color_mode(ColorMode::Always));
}

TEST(ColorModeTest, NeverReturnsFalse) {
    EXPECT_FALSE(computo::resolve_color_mode(ColorMode::Never));
}
