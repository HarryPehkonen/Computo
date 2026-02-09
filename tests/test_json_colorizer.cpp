#include "computo.hpp"
#include "json_colorizer.hpp"
#include <gtest/gtest.h>
#include <regex>
#include <string>

using computo::ColorMode;
using computo::ScriptColorTheme;
using computo::ScriptColorizer;

// Helper: strip all ANSI escape codes from a string
static auto strip_ansi(const std::string& input) -> std::string {
    static const std::regex ansi_re("\033\\[[0-9;]*m");
    return std::regex_replace(input, ansi_re, "");
}

// Helper: check if a substring is wrapped with expected ANSI code
static auto contains_colored(const std::string& output, const char* ansi_code,
                              const std::string& text) -> bool {
    std::string pattern = std::string(ansi_code) + text + "\033[0m";
    return output.find(pattern) != std::string::npos;
}

class ScriptColorizerTest : public ::testing::Test {
protected:
    ScriptColorTheme theme = ScriptColorTheme::default_theme();
};

// --- Operator coloring ---

TEST_F(ScriptColorizerTest, OperatorIsBoldCyan) {
    auto doc = jsom::parse_document(R"(["+", 1, 2])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.op, "\"+\""));
}

TEST_F(ScriptColorizerTest, MapOperatorIsBoldCyan) {
    auto doc = jsom::parse_document(R"(["map", [1, 2], ["lambda", ["x"], ["$", "/x"]]])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.op, "\"map\""));
}

// --- Lambda coloring ---

TEST_F(ScriptColorizerTest, LambdaKeywordIsBoldMagenta) {
    auto doc = jsom::parse_document(R"(["lambda", ["x"], ["+", ["$", "/x"], 1]])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.lambda_kw, "\"lambda\""));
}

TEST_F(ScriptColorizerTest, LambdaParamsAreYellow) {
    auto doc = jsom::parse_document(R"(["lambda", ["a", "b"], ["+", ["$", "/a"], ["$", "/b"]]])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.param, "\"a\""));
    EXPECT_TRUE(contains_colored(result, theme.param, "\"b\""));
}

// --- Variable access coloring ---

TEST_F(ScriptColorizerTest, DollarIsGreen) {
    auto doc = jsom::parse_document(R"(["$", "/x"])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.var_access, "\"$\""));
}

TEST_F(ScriptColorizerTest, DollarInputIsGreen) {
    auto doc = jsom::parse_document(R"(["$input", "/name"])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.var_access, "\"$input\""));
}

TEST_F(ScriptColorizerTest, DollarInputsIsGreen) {
    auto doc = jsom::parse_document(R"(["$inputs", "/0"])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.var_access, "\"$inputs\""));
}

// --- JSON pointer coloring ---

TEST_F(ScriptColorizerTest, JsonPointerIsGreenUnderline) {
    auto doc = jsom::parse_document(R"(["$", "/users/name"])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.pointer, "\"/users/name\""));
}

// --- Number coloring ---

TEST_F(ScriptColorizerTest, NumberIsBrightWhite) {
    auto doc = jsom::parse_document(R"(["+", 1, 2])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.number, "1"));
    EXPECT_TRUE(contains_colored(result, theme.number, "2"));
}

// --- Boolean/null coloring ---

TEST_F(ScriptColorizerTest, TrueIsRed) {
    auto doc = jsom::parse_document("true");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.bool_null, "true"));
}

TEST_F(ScriptColorizerTest, FalseIsRed) {
    auto doc = jsom::parse_document("false");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.bool_null, "false"));
}

TEST_F(ScriptColorizerTest, NullIsRed) {
    auto doc = jsom::parse_document("null");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.bool_null, "null"));
}

// --- Array wrapper coloring ---

TEST_F(ScriptColorizerTest, ArrayWrapperKeyIsDim) {
    auto doc = jsom::parse_document(R"({"array": [1, 2, 3]})");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.array_wrapper, "\"array\""));
}

TEST_F(ScriptColorizerTest, CustomArrayKeyIsDim) {
    auto doc = jsom::parse_document(R"({"@data": [1, 2, 3]})");
    auto result = ScriptColorizer::colorize(doc, theme, "@data");
    EXPECT_TRUE(contains_colored(result, theme.array_wrapper, "\"@data\""));
}

// --- Structural chars ---

TEST_F(ScriptColorizerTest, BracketsAreDim) {
    auto doc = jsom::parse_document(R"(["+", 1, 2])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.structural, "["));
    EXPECT_TRUE(contains_colored(result, theme.structural, "]"));
}

// --- Nested structures ---

TEST_F(ScriptColorizerTest, NestedOperatorCalls) {
    auto doc = jsom::parse_document(R"(["+", ["*", 2, 3], 4])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.op, "\"+\""));
    EXPECT_TRUE(contains_colored(result, theme.op, "\"*\""));
}

TEST_F(ScriptColorizerTest, LambdaInsideMap) {
    auto doc = jsom::parse_document(R"(["map", [1, 2], ["lambda", ["x"], ["*", ["$", "/x"], 2]]])");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_TRUE(contains_colored(result, theme.op, "\"map\""));
    EXPECT_TRUE(contains_colored(result, theme.lambda_kw, "\"lambda\""));
    EXPECT_TRUE(contains_colored(result, theme.param, "\"x\""));
    EXPECT_TRUE(contains_colored(result, theme.var_access, "\"$\""));
    EXPECT_TRUE(contains_colored(result, theme.pointer, "\"/x\""));
}

// --- Edge cases ---

TEST_F(ScriptColorizerTest, EmptyArray) {
    auto doc = jsom::parse_document("[]");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_EQ(strip_ansi(result), "[]");
}

TEST_F(ScriptColorizerTest, EmptyObject) {
    auto doc = jsom::parse_document("{}");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_EQ(strip_ansi(result), "{}");
}

TEST_F(ScriptColorizerTest, TopLevelString) {
    auto doc = jsom::parse_document("\"hello\"");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_EQ(strip_ansi(result), "\"hello\"");
}

TEST_F(ScriptColorizerTest, TopLevelNumber) {
    auto doc = jsom::parse_document("42");
    auto result = ScriptColorizer::colorize(doc, theme);
    EXPECT_EQ(strip_ansi(result), "42");
}

// --- Roundtrip: stripping ANSI yields valid JSON ---

TEST_F(ScriptColorizerTest, RoundtripSimpleExpression) {
    auto doc = jsom::parse_document(R"(["+", 1, 2])");
    auto result = ScriptColorizer::colorize(doc, theme);
    auto stripped = strip_ansi(result);
    // Should parse as valid JSON
    auto reparsed = jsom::parse_document(stripped);
    EXPECT_TRUE(reparsed.is_array());
    EXPECT_EQ(reparsed.size(), 3);
}

TEST_F(ScriptColorizerTest, RoundtripComplexScript) {
    std::string json = R"(["let", [["x", 10], ["y", 20]], ["+", ["$", "/x"], ["$", "/y"]]])";
    auto doc = jsom::parse_document(json);
    auto result = ScriptColorizer::colorize(doc, theme);
    auto stripped = strip_ansi(result);
    auto reparsed = jsom::parse_document(stripped);
    EXPECT_TRUE(reparsed.is_array());
    EXPECT_EQ((reparsed[0].as<std::string>()), "let");
}

TEST_F(ScriptColorizerTest, RoundtripWithObject) {
    auto doc = jsom::parse_document(R"(["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])");
    auto result = ScriptColorizer::colorize(doc, theme);
    auto stripped = strip_ansi(result);
    auto reparsed = jsom::parse_document(stripped);
    EXPECT_TRUE(reparsed.is_array());
    EXPECT_EQ((reparsed[0].as<std::string>()), "map");
}

// --- Semantic formatting structure tests (using no_color theme for easy assertions) ---

class FormattingTest : public ::testing::Test {
protected:
    ScriptColorTheme plain = ScriptColorTheme::no_color();

    auto format(const std::string& json) -> std::string {
        return ScriptColorizer::colorize(jsom::parse_document(json), plain);
    }

    // Check that a line at the given indentation level contains the expected text
    static auto has_line(const std::string& output, int indent_level,
                         const std::string& content) -> bool {
        std::string prefix;
        for (int idx = 0; idx < indent_level; ++idx) {
            prefix += "  ";
        }
        std::string target = prefix + content;
        // Check for the line at start or after a newline
        if (output.find(target) == 0) { return true; }
        return output.find("\n" + target) != std::string::npos;
    }
};

TEST_F(FormattingTest, ShortExpressionStaysInline) {
    auto result = format(R"(["+", 1, 2])");
    EXPECT_EQ(result.find('\n'), std::string::npos) << "Short expression should be single line";
}

TEST_F(FormattingTest, LetBindingsOnSeparateLines) {
    auto result = format(R"(["let", [["x", 10], ["y", 20]], ["+", ["$", "/x"], ["$", "/y"]]])");
    EXPECT_TRUE(has_line(result, 0, "[\"let\",")) << "let on first line";
    EXPECT_TRUE(has_line(result, 2, "[\"x\",")) << "first binding indented";
    EXPECT_TRUE(has_line(result, 2, "[\"y\",")) << "second binding indented";
    EXPECT_TRUE(has_line(result, 1, "[\"+\"")) << "body indented under let";
}

TEST_F(FormattingTest, LetBodyAtEnd) {
    auto result = format(R"(["let", [["x", 10]], ["$", "/x"]])");
    // Body should appear after the bindings array closes
    auto bindings_close = result.find("],\n");
    EXPECT_NE(bindings_close, std::string::npos);
    auto body_pos = result.find("[\"$\"", bindings_close);
    EXPECT_NE(body_pos, std::string::npos) << "Body should follow bindings";
}

TEST_F(FormattingTest, LambdaShortBodyInline) {
    auto result = format(R"(["lambda", ["x"], ["+", ["$", "/x"], 1]])");
    // Short lambda should fit on one line (no wrapping of body)
    auto reparsed = jsom::parse_document(result);
    EXPECT_TRUE(reparsed.is_array());
    EXPECT_EQ((reparsed[0].as<std::string>()), "lambda");
}

TEST_F(FormattingTest, LambdaLongBodyWraps) {
    // Make a body long enough to trigger wrapping
    auto result = format(
        R"(["lambda", ["x"], ["+", ["*", ["$", "/x"], ["$", "/x"]], ["*", ["$", "/x"], 100]]])"
    );
    EXPECT_NE(result.find('\n'), std::string::npos) << "Long lambda body should wrap";
    EXPECT_TRUE(has_line(result, 0, "[\"lambda\", [\"x\"],"))
        << "lambda + params on first line";
}

TEST_F(FormattingTest, MapPutsLambdaOnIndentedLine) {
    auto result = format(
        R"(["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])"
    );
    EXPECT_NE(result.find('\n'), std::string::npos) << "Should be multiline";
    EXPECT_TRUE(has_line(result, 0, "[\"map\",")) << "map on first line";
    EXPECT_TRUE(has_line(result, 1, "[\"lambda\"")) << "lambda indented on own line";
}

TEST_F(FormattingTest, FilterPutsLambdaOnIndentedLine) {
    auto result = format(
        R"(["filter", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]])"
    );
    EXPECT_TRUE(has_line(result, 0, "[\"filter\",")) << "filter on first line";
    EXPECT_TRUE(has_line(result, 1, "[\"lambda\"")) << "lambda indented on own line";
}

TEST_F(FormattingTest, ReduceInitialValueOnOwnLine) {
    auto result = format(
        R"(["reduce", {"array": [1, 2, 3]}, ["lambda", ["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]], 0])"
    );
    EXPECT_TRUE(has_line(result, 1, "[\"lambda\"")) << "lambda indented";
    EXPECT_TRUE(has_line(result, 1, "0")) << "initial value on own indented line";
}

TEST_F(FormattingTest, IfBranchesOnSeparateLines) {
    auto result = format(
        R"(["if", [">", ["$", "/x"], 0], ["*", ["$", "/x"], 2], ["*", ["$", "/x"], -1]])"
    );
    EXPECT_NE(result.find('\n'), std::string::npos) << "Should be multiline";
    EXPECT_TRUE(has_line(result, 0, "[\"if\",")) << "if on first line";
}

TEST_F(FormattingTest, IfShortStaysInline) {
    auto result = format(R"(["if", true, 1, 0])");
    EXPECT_EQ(result.find('\n'), std::string::npos) << "Short if should be single line";
}

TEST_F(FormattingTest, ClosingBracketsAligned) {
    auto result = format(
        R"(["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])"
    );
    // The closing ] of the map should be at indent 0
    EXPECT_TRUE(result.back() == ']' || result.substr(result.size() - 1) == "]");
    EXPECT_TRUE(has_line(result, 0, "]")) << "Closing bracket at indent 0";
}

TEST_F(FormattingTest, FormattedOutputIsValidJson) {
    // The proposal's example script
    std::string json = R"(["let",[["active",["filter",["$input","/users"],["lambda",["u"],[">",["count",["$","/u/orders"]],0]]]],["totals",["map",["$","/active"],["lambda",["u"],["reduce",["$","/u/orders"],["lambda",["a","o"],["+",["$","/a"],["$","/o/total"]]],0]]]]],["$","/totals"]])";
    auto result = format(json);
    // Must parse back as valid JSON
    auto reparsed = jsom::parse_document(result);
    EXPECT_TRUE(reparsed.is_array());
    EXPECT_EQ((reparsed[0].as<std::string>()), "let");
    EXPECT_EQ(reparsed.size(), 3);
}

TEST_F(FormattingTest, NoColorThemeProducesNoAnsiCodes) {
    std::string json = R"(["let", [["x", 10]], ["+", ["$", "/x"], 1]])";
    auto result = format(json);
    EXPECT_EQ(result.find('\033'), std::string::npos) << "No ANSI codes in no_color output";
}

// --- resolve_color_mode ---

TEST(ColorModeTest, AlwaysReturnsTrue) {
    EXPECT_TRUE(computo::resolve_color_mode(ColorMode::Always));
}

TEST(ColorModeTest, NeverReturnsFalse) {
    EXPECT_FALSE(computo::resolve_color_mode(ColorMode::Never));
}
