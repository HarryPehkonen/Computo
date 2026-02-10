#include "sugar_writer.hpp"
#include <gtest/gtest.h>
#include <jsom/jsom.hpp>

using computo::SugarWriter;
using computo::SugarWriterOptions;

class SugarWriterTest : public ::testing::Test {
protected:
    SugarWriterOptions opts;

    auto write(const std::string& json) -> std::string {
        return SugarWriter::write(jsom::parse_document(json), opts);
    }
};

// --- Literals ---

TEST_F(SugarWriterTest, NumberLiteral) {
    EXPECT_EQ(write("42"), "42");
}

TEST_F(SugarWriterTest, FloatLiteral) {
    EXPECT_EQ(write("3.14"), "3.14");
}

TEST_F(SugarWriterTest, StringLiteral) {
    EXPECT_EQ(write(R"("hello")"), "\"hello\"");
}

TEST_F(SugarWriterTest, TrueLiteral) {
    EXPECT_EQ(write("true"), "true");
}

TEST_F(SugarWriterTest, FalseLiteral) {
    EXPECT_EQ(write("false"), "false");
}

TEST_F(SugarWriterTest, NullLiteral) {
    EXPECT_EQ(write("null"), "null");
}

// --- Variable access ---

TEST_F(SugarWriterTest, SimpleVariable) {
    EXPECT_EQ(write(R"(["$", "/x"])"), "x");
}

TEST_F(SugarWriterTest, NestedVariablePath) {
    EXPECT_EQ(write(R"(["$", "/user/name"])"), "user/name");
}

TEST_F(SugarWriterTest, DeepVariablePath) {
    EXPECT_EQ(write(R"(["$", "/a/b/c/d"])"), "a/b/c/d");
}

// --- Input access ---

TEST_F(SugarWriterTest, InputNoPath) {
    EXPECT_EQ(write(R"(["$input"])"), "$input");
}

TEST_F(SugarWriterTest, InputWithPath) {
    EXPECT_EQ(write(R"(["$input", "/users"])"), "$input/users");
}

TEST_F(SugarWriterTest, InputsWithPath) {
    EXPECT_EQ(write(R"(["$inputs", "/0/name"])"), "$inputs/0/name");
}

// --- Infix operators ---

TEST_F(SugarWriterTest, Addition) {
    EXPECT_EQ(write(R"(["+", 1, 2])"), "1 + 2");
}

TEST_F(SugarWriterTest, Subtraction) {
    EXPECT_EQ(write(R"(["-", 10, 3])"), "10 - 3");
}

TEST_F(SugarWriterTest, Multiplication) {
    EXPECT_EQ(write(R"(["*", 2, 3])"), "2 * 3");
}

TEST_F(SugarWriterTest, Division) {
    EXPECT_EQ(write(R"(["/", 10, 2])"), "10 / 2");
}

TEST_F(SugarWriterTest, Modulo) {
    EXPECT_EQ(write(R"(["%", 10, 3])"), "10 % 3");
}

TEST_F(SugarWriterTest, GreaterThan) {
    EXPECT_EQ(write(R"([">", 5, 3])"), "5 > 3");
}

TEST_F(SugarWriterTest, LessEqual) {
    EXPECT_EQ(write(R"(["<=", 1, 2])"), "1 <= 2");
}

TEST_F(SugarWriterTest, EqualEqual) {
    EXPECT_EQ(write(R"(["==", 1, 1])"), "1 == 1");
}

TEST_F(SugarWriterTest, NotEqual) {
    EXPECT_EQ(write(R"(["!=", 1, 2])"), "1 != 2");
}

TEST_F(SugarWriterTest, LogicalAnd) {
    EXPECT_EQ(write(R"(["and", true, false])"), "true and false");
}

TEST_F(SugarWriterTest, LogicalOr) {
    EXPECT_EQ(write(R"(["or", true, false])"), "true or false");
}

// --- Variadic infix ---

TEST_F(SugarWriterTest, VariadicAddition) {
    EXPECT_EQ(write(R"(["+", 1, 2, 3])"), "1 + 2 + 3");
}

TEST_F(SugarWriterTest, ChainedComparison) {
    EXPECT_EQ(write(R"([">", 5, 3, 1])"), "5 > 3 > 1");
}

// --- Precedence / parenthesization ---

TEST_F(SugarWriterTest, MulInsideAdd) {
    EXPECT_EQ(write(R"(["+", ["*", 2, 3], 4])"), "2 * 3 + 4");
}

TEST_F(SugarWriterTest, AddInsideMulNeedsParens) {
    EXPECT_EQ(write(R"(["*", ["+", 1, 2], 3])"), "(1 + 2) * 3");
}

TEST_F(SugarWriterTest, NestedSamePrecFlattensSameOp) {
    // ["+", ["+", 1, 2], 3] - inner add is lower-or-equal prec to outer add,
    // but since Computo evaluates + as N-ary, no parens needed for same operator
    EXPECT_EQ(write(R"(["+", ["+", 1, 2], 3])"), "1 + 2 + 3");
}

TEST_F(SugarWriterTest, ComparisonInsideAnd) {
    EXPECT_EQ(write(R"(["and", [">", 5, 3], ["<", 1, 10]])"), "5 > 3 and 1 < 10");
}

// --- Unary operators ---

TEST_F(SugarWriterTest, UnaryMinus) {
    EXPECT_EQ(write(R"(["-", 5])"), "-5");
}

TEST_F(SugarWriterTest, NotOperator) {
    EXPECT_EQ(write(R"(["not", true])"), "not true");
}

// --- Lambda ---

TEST_F(SugarWriterTest, SimpleLambda) {
    EXPECT_EQ(write(R"(["lambda", ["x"], ["+", ["$", "/x"], 1]])"),
              "(x) => x + 1");
}

TEST_F(SugarWriterTest, MultiParamLambda) {
    EXPECT_EQ(write(R"(["lambda", ["a", "b"], ["+", ["$", "/a"], ["$", "/b"]]])"),
              "(a, b) => a + b");
}

TEST_F(SugarWriterTest, ZeroParamLambda) {
    EXPECT_EQ(write(R"(["lambda", [], 42])"), "() => 42");
}

// --- Let ---

TEST_F(SugarWriterTest, SimpleLetBinding) {
    auto result = write(R"(["let", [["x", 10]], ["$", "/x"]])");
    EXPECT_NE(result.find("let"), std::string::npos);
    EXPECT_NE(result.find("x = 10"), std::string::npos);
    EXPECT_NE(result.find("in"), std::string::npos);
}

TEST_F(SugarWriterTest, MultipleLetBindings) {
    auto result = write(R"(["let", [["x", 10], ["y", 20]], ["+", ["$", "/x"], ["$", "/y"]]])");
    EXPECT_NE(result.find("x = 10"), std::string::npos);
    EXPECT_NE(result.find("y = 20"), std::string::npos);
}

// --- If ---

TEST_F(SugarWriterTest, SimpleIf) {
    EXPECT_EQ(write(R"(["if", true, 1, 0])"),
              "if true then 1 else 0");
}

TEST_F(SugarWriterTest, IfWithExpressions) {
    auto result = write(R"(["if", [">", 5, 3], "yes", "no"])");
    EXPECT_EQ(result, "if 5 > 3 then \"yes\" else \"no\"");
}

// --- Function calls ---

TEST_F(SugarWriterTest, SingleArgFunction) {
    EXPECT_EQ(write(R"(["count", ["$", "/arr"]])"), "count(arr)");
}

TEST_F(SugarWriterTest, MultiArgFunction) {
    EXPECT_EQ(write(R"(["map", ["$", "/arr"], ["lambda", ["x"], ["$", "/x"]]])"),
              "map(arr, (x) => x)");
}

TEST_F(SugarWriterTest, NoArgFunction) {
    EXPECT_EQ(write(R"(["someOp"])"), "someOp()");
}

// --- Array literals ---

TEST_F(SugarWriterTest, EmptyArrayLiteral) {
    EXPECT_EQ(write(R"({"array": []})"), "[]");
}

TEST_F(SugarWriterTest, ArrayLiteral) {
    EXPECT_EQ(write(R"({"array": [1, 2, 3]})"), "[1, 2, 3]");
}

TEST_F(SugarWriterTest, CustomArrayKey) {
    opts.array_key = "@data";
    auto doc = jsom::parse_document(R"({"@data": [1, 2]})");
    EXPECT_EQ(SugarWriter::write(doc, opts), "[1, 2]");
}

// --- Object literals ---

TEST_F(SugarWriterTest, EmptyObject) {
    EXPECT_EQ(write("{}"), "{}");
}

TEST_F(SugarWriterTest, SimpleObject) {
    // std::map orders keys alphabetically, so age comes before name
    EXPECT_EQ(write(R"({"name": "Alice", "age": 30})"), "{age: 30, name: \"Alice\"}");
}

// --- Complex expressions ---

TEST_F(SugarWriterTest, FilterWithLambda) {
    EXPECT_EQ(write(R"(["filter", ["$input", "/users"], ["lambda", ["u"], [">", ["count", ["$", "/u/orders"]], 0]]])"),
              "filter($input/users, (u) => count(u/orders) > 0)");
}

TEST_F(SugarWriterTest, NestedFunctionCalls) {
    EXPECT_EQ(write(R"(["count", ["filter", ["$", "/arr"], ["lambda", ["x"], [">", ["$", "/x"], 0]]]])"),
              "count(filter(arr, (x) => x > 0))");
}
