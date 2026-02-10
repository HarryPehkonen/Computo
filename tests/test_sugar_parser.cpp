#include "sugar_parser.hpp"
#include <gtest/gtest.h>
#include <jsom/jsom.hpp>

using computo::SugarParser;
using computo::SugarParseError;
using computo::SugarParseOptions;
using json = jsom::JsonDocument;

class SugarParserTest : public ::testing::Test {
protected:
    SugarParseOptions opts;

    auto parse(const std::string& source) -> json {
        return SugarParser::parse(source, opts);
    }
};

// --- Literals ---

TEST_F(SugarParserTest, NumberLiteral) {
    EXPECT_EQ(parse("42"), json(42));
}

TEST_F(SugarParserTest, FloatLiteral) {
    EXPECT_EQ(parse("3.14"), json(3.14));
}

TEST_F(SugarParserTest, StringLiteral) {
    EXPECT_EQ(parse("\"hello\""), json("hello"));
}

TEST_F(SugarParserTest, TrueLiteral) {
    EXPECT_EQ(parse("true"), json(true));
}

TEST_F(SugarParserTest, FalseLiteral) {
    EXPECT_EQ(parse("false"), json(false));
}

TEST_F(SugarParserTest, NullLiteral) {
    EXPECT_EQ(parse("null"), json(nullptr));
}

// --- Variable access ---

TEST_F(SugarParserTest, SimpleVariable) {
    auto result = parse("x");
    auto expected = jsom::parse_document(R"(["$", "/x"])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, VariableWithPath) {
    auto result = parse("user/name");
    auto expected = jsom::parse_document(R"(["$", "/user/name"])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, DeepPath) {
    auto result = parse("a/b/c/d");
    auto expected = jsom::parse_document(R"(["$", "/a/b/c/d"])");
    EXPECT_EQ(result, expected);
}

// --- Input access ---

TEST_F(SugarParserTest, DollarInputNoPath) {
    auto result = parse("$input");
    auto expected = jsom::parse_document(R"(["$input"])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, DollarInputWithPath) {
    auto result = parse("$input/users");
    auto expected = jsom::parse_document(R"(["$input", "/users"])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, DollarInputDeepPath) {
    auto result = parse("$input/users/0/name");
    auto expected = jsom::parse_document(R"(["$input", "/users/0/name"])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, DollarInputsWithPath) {
    auto result = parse("$inputs/0");
    auto expected = jsom::parse_document(R"(["$inputs", "/0"])");
    EXPECT_EQ(result, expected);
}

// --- Infix operators ---

TEST_F(SugarParserTest, Addition) {
    auto result = parse("1 + 2");
    auto expected = jsom::parse_document(R"(["+", 1, 2])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, Subtraction) {
    auto result = parse("10 - 3");
    auto expected = jsom::parse_document(R"(["-", 10, 3])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, Multiplication) {
    auto result = parse("2 * 3");
    auto expected = jsom::parse_document(R"(["*", 2, 3])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, DivisionWithSpaces) {
    auto result = parse("10 / 2");
    auto expected = jsom::parse_document(R"(["/", 10, 2])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, Modulo) {
    auto result = parse("10 % 3");
    auto expected = jsom::parse_document(R"(["%", 10, 3])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, GreaterThan) {
    auto result = parse("5 > 3");
    auto expected = jsom::parse_document(R"([">", 5, 3])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, LessEqual) {
    auto result = parse("1 <= 2");
    auto expected = jsom::parse_document(R"(["<=", 1, 2])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, EqualEqual) {
    auto result = parse("1 == 1");
    auto expected = jsom::parse_document(R"(["==", 1, 1])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, NotEqual) {
    auto result = parse("1 != 2");
    auto expected = jsom::parse_document(R"(["!=", 1, 2])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, LogicalAnd) {
    auto result = parse("true and false");
    auto expected = jsom::parse_document(R"(["and", true, false])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, LogicalOr) {
    auto result = parse("true or false");
    auto expected = jsom::parse_document(R"(["or", true, false])");
    EXPECT_EQ(result, expected);
}

// --- Variadic flattening ---

TEST_F(SugarParserTest, VariadicAddition) {
    auto result = parse("1 + 2 + 3");
    auto expected = jsom::parse_document(R"(["+", 1, 2, 3])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, VariadicMultiplication) {
    auto result = parse("2 * 3 * 4");
    auto expected = jsom::parse_document(R"(["*", 2, 3, 4])");
    EXPECT_EQ(result, expected);
}

// --- Precedence ---

TEST_F(SugarParserTest, MulHigherThanAdd) {
    auto result = parse("1 + 2 * 3");
    auto expected = jsom::parse_document(R"(["+", 1, ["*", 2, 3]])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, ParensOverridePrecedence) {
    auto result = parse("(1 + 2) * 3");
    auto expected = jsom::parse_document(R"(["*", ["+", 1, 2], 3])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, ComparisonLowerThanArithmetic) {
    auto result = parse("x + 1 > 5");
    auto expected = jsom::parse_document(R"([">", ["+", ["$", "/x"], 1], 5])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, AndLowerThanComparison) {
    auto result = parse("x > 0 and x < 10");
    auto expected = jsom::parse_document(R"(["and", [">", ["$", "/x"], 0], ["<", ["$", "/x"], 10]])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, OrLowerThanAnd) {
    auto result = parse("a and b or c");
    auto expected = jsom::parse_document(R"(["or", ["and", ["$", "/a"], ["$", "/b"]], ["$", "/c"]])");
    EXPECT_EQ(result, expected);
}

// --- Unary operators ---

TEST_F(SugarParserTest, UnaryMinus) {
    auto result = parse("-5");
    auto expected = jsom::parse_document(R"(["-", 5])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, NotOperator) {
    auto result = parse("not true");
    auto expected = jsom::parse_document(R"(["not", true])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, NotWithExpression) {
    auto result = parse("not x > 5");
    // not binds tighter than comparison? No - not has prec 4, comparison is 5.
    // "not x > 5" should parse as "not (x > 5)"? Let's check:
    // not has prec 4, and we parse its operand with min_prec = 5
    // So "x > 5" at prec 5 will be consumed: > has prec 5 >= 5, so yes.
    // Result: ["not", [">", x, 5]]
    auto expected = jsom::parse_document(R"(["not", [">", ["$", "/x"], 5]])");
    EXPECT_EQ(result, expected);
}

// --- Lambda ---

TEST_F(SugarParserTest, SimpleLambda) {
    auto result = parse("(x) => x + 1");
    auto expected = jsom::parse_document(R"(["lambda", ["x"], ["+", ["$", "/x"], 1]])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, MultiParamLambda) {
    auto result = parse("(a, b) => a + b");
    auto expected = jsom::parse_document(R"(["lambda", ["a", "b"], ["+", ["$", "/a"], ["$", "/b"]]])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, ZeroParamLambda) {
    auto result = parse("() => 42");
    auto expected = jsom::parse_document(R"(["lambda", [], 42])");
    EXPECT_EQ(result, expected);
}

// --- Grouping parens ---

TEST_F(SugarParserTest, GroupedExpression) {
    auto result = parse("(1 + 2)");
    auto expected = jsom::parse_document(R"(["+", 1, 2])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, GroupedVariable) {
    auto result = parse("(x)");
    auto expected = jsom::parse_document(R"(["$", "/x"])");
    EXPECT_EQ(result, expected);
}

// --- Let ---

TEST_F(SugarParserTest, SimpleLetBinding) {
    auto result = parse("let x = 10 in x");
    auto expected = jsom::parse_document(R"(["let", [["x", 10]], ["$", "/x"]])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, MultipleLetBindings) {
    auto result = parse("let x = 10, y = 20 in x + y");
    auto expected = jsom::parse_document(
        R"(["let", [["x", 10], ["y", 20]], ["+", ["$", "/x"], ["$", "/y"]]])"
    );
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, LetWithComplexBody) {
    auto result = parse("let arr = $input/items in count(arr)");
    auto expected = jsom::parse_document(
        R"(["let", [["arr", ["$input", "/items"]]], ["count", ["$", "/arr"]]])"
    );
    EXPECT_EQ(result, expected);
}

// --- If ---

TEST_F(SugarParserTest, SimpleIf) {
    auto result = parse("if true then 1 else 0");
    auto expected = jsom::parse_document(R"(["if", true, 1, 0])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, IfWithExpressions) {
    auto result = parse("if x > 0 then x else -x");
    auto expected = jsom::parse_document(
        R"(["if", [">", ["$", "/x"], 0], ["$", "/x"], ["-", ["$", "/x"]]])"
    );
    EXPECT_EQ(result, expected);
}

// --- Function calls ---

TEST_F(SugarParserTest, SingleArgFunction) {
    auto result = parse("count(arr)");
    auto expected = jsom::parse_document(R"(["count", ["$", "/arr"]])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, MultiArgFunction) {
    auto result = parse("filter(arr, (x) => x > 0)");
    auto expected = jsom::parse_document(
        R"(["filter", ["$", "/arr"], ["lambda", ["x"], [">", ["$", "/x"], 0]]])"
    );
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, NoArgFunction) {
    auto result = parse("someOp()");
    auto expected = jsom::parse_document(R"(["someOp"])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, NestedFunctionCalls) {
    auto result = parse("count(filter(arr, (x) => x > 0))");
    auto expected = jsom::parse_document(
        R"(["count", ["filter", ["$", "/arr"], ["lambda", ["x"], [">", ["$", "/x"], 0]]]])"
    );
    EXPECT_EQ(result, expected);
}

// --- Array literals ---

TEST_F(SugarParserTest, EmptyArrayLiteral) {
    auto result = parse("[]");
    auto expected = jsom::parse_document(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, ArrayLiteral) {
    auto result = parse("[1, 2, 3]");
    auto expected = jsom::parse_document(R"({"array": [1, 2, 3]})");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, ArrayWithExpressions) {
    auto result = parse("[1 + 2, 3 * 4]");
    auto expected = jsom::parse_document(R"({"array": [["+", 1, 2], ["*", 3, 4]]})");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, CustomArrayKey) {
    opts.array_key = "@data";
    auto result = parse("[1, 2]");
    auto expected = jsom::parse_document(R"({"@data": [1, 2]})");
    EXPECT_EQ(result, expected);
}

// --- Object literals ---

TEST_F(SugarParserTest, EmptyObject) {
    auto result = parse("{}");
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.empty());
}

TEST_F(SugarParserTest, SimpleObject) {
    auto result = parse("{name: \"Alice\", age: 30}");
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result["name"], json("Alice"));
    EXPECT_EQ(result["age"], json(30));
}

TEST_F(SugarParserTest, ObjectWithStringKeys) {
    auto result = parse("{\"first name\": \"Alice\"}");
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result["first name"], json("Alice"));
}

// --- Comments ---

TEST_F(SugarParserTest, LineComment) {
    auto result = parse("-- this is a comment\n42");
    EXPECT_EQ(result, json(42));
}

TEST_F(SugarParserTest, InlineComment) {
    auto result = parse("1 + 2 -- add things");
    auto expected = jsom::parse_document(R"(["+", 1, 2])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, MultiLineComments) {
    auto result = parse("-- first\n-- second\n42");
    EXPECT_EQ(result, json(42));
}

// --- Shebang ---

TEST_F(SugarParserTest, ShebangLine) {
    auto result = parse("#!/usr/bin/env -S computo --script\n42");
    EXPECT_EQ(result, json(42));
}

// --- Slash ambiguity ---

TEST_F(SugarParserTest, SlashAsPathNoSpace) {
    // x/name -> path access (no spaces)
    auto result = parse("x/name");
    auto expected = jsom::parse_document(R"(["$", "/x/name"])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, SlashAsDivisionWithSpaces) {
    // x / 2 -> division (spaces on both sides)
    auto result = parse("x / 2");
    auto expected = jsom::parse_document(R"(["/", ["$", "/x"], 2])");
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, SlashAsymmetricSpaceLeftError) {
    // x /2 -> error (space only on left)
    EXPECT_THROW(parse("x /2"), SugarParseError);
}

TEST_F(SugarParserTest, SlashAsymmetricSpaceRightError) {
    // x/ 2 -> error (space only on right)
    EXPECT_THROW(parse("x/ 2"), SugarParseError);
}

// --- Error cases ---

TEST_F(SugarParserTest, UnexpectedToken) {
    EXPECT_THROW(parse("@"), SugarParseError);
}

TEST_F(SugarParserTest, MissingThen) {
    EXPECT_THROW(parse("if true 1 else 0"), SugarParseError);
}

TEST_F(SugarParserTest, MissingElse) {
    EXPECT_THROW(parse("if true then 1"), SugarParseError);
}

TEST_F(SugarParserTest, MissingIn) {
    EXPECT_THROW(parse("let x = 10 x"), SugarParseError);
}

TEST_F(SugarParserTest, ErrorHasLineAndColumn) {
    try {
        parse("let x = 10 @");
        FAIL() << "Expected SugarParseError";
    } catch (const SugarParseError& e) {
        EXPECT_GE(e.line, 1);
        EXPECT_GE(e.column, 1);
    }
}

// --- Complex expressions ---

TEST_F(SugarParserTest, FilterCountExample) {
    auto result = parse("filter($input/users, (u) => count(u/orders) > 0)");
    auto expected = jsom::parse_document(
        R"(["filter", ["$input", "/users"], ["lambda", ["u"], [">", ["count", ["$", "/u/orders"]], 0]]])"
    );
    EXPECT_EQ(result, expected);
}

TEST_F(SugarParserTest, LetWithFilterAndMap) {
    auto result = parse(R"(
        let
          active = filter($input/users, (u) => count(u/orders) > 0)
        in
          map(active, (u) => u/name)
    )");
    // Verify structure
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result[0].as<std::string>(), "let");
    EXPECT_TRUE(result[1].is_array()); // bindings
    EXPECT_EQ(result[1].size(), 1);    // one binding
    EXPECT_TRUE(result[2].is_array()); // body (map call)
    EXPECT_EQ(result[2][0].as<std::string>(), "map");
}
