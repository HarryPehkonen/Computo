#include "sugar_parser.hpp"
#include "sugar_writer.hpp"
#include <computo.hpp>
#include <gtest/gtest.h>
#include <jsom/jsom.hpp>

using computo::SugarParser;
using computo::SugarWriter;
using computo::SugarParseOptions;
using computo::SugarWriterOptions;
using json = jsom::JsonDocument;

class SugarRoundtripTest : public ::testing::Test {
protected:
    SugarParseOptions parse_opts;
    SugarWriterOptions write_opts;

    // AST -> sugar -> AST roundtrip
    void check_ast_roundtrip(const std::string& json_str) {
        auto original = jsom::parse_document(json_str);
        auto sugar = SugarWriter::write(original, write_opts);
        auto reparsed = SugarParser::parse(sugar, parse_opts);
        EXPECT_EQ(reparsed, original) << "AST -> sugar -> AST failed.\n"
            << "Original JSON: " << json_str << "\n"
            << "Sugar: " << sugar << "\n"
            << "Reparsed JSON: " << reparsed.to_json();
    }

    // Sugar -> AST -> sugar roundtrip
    void check_sugar_roundtrip(const std::string& sugar_str) {
        auto ast = SugarParser::parse(sugar_str, parse_opts);
        auto sugar = SugarWriter::write(ast, write_opts);
        auto reparsed = SugarParser::parse(sugar, parse_opts);
        EXPECT_EQ(reparsed, ast) << "Sugar -> AST -> sugar -> AST failed.\n"
            << "Original sugar: " << sugar_str << "\n"
            << "Written sugar: " << sugar << "\n"
            << "AST: " << ast.to_json() << "\n"
            << "Reparsed AST: " << reparsed.to_json();
    }
};

// --- AST -> Sugar -> AST roundtrips ---

TEST_F(SugarRoundtripTest, NumberLiteral) {
    check_ast_roundtrip("42");
}

TEST_F(SugarRoundtripTest, StringLiteral) {
    check_ast_roundtrip(R"("hello")");
}

TEST_F(SugarRoundtripTest, BooleanLiterals) {
    check_ast_roundtrip("true");
    check_ast_roundtrip("false");
}

TEST_F(SugarRoundtripTest, NullLiteral) {
    check_ast_roundtrip("null");
}

TEST_F(SugarRoundtripTest, VariableAccess) {
    check_ast_roundtrip(R"(["$", "/x"])");
}

TEST_F(SugarRoundtripTest, NestedVariablePath) {
    check_ast_roundtrip(R"(["$", "/user/name"])");
}

TEST_F(SugarRoundtripTest, InputAccess) {
    check_ast_roundtrip(R"(["$input", "/users"])");
}

TEST_F(SugarRoundtripTest, InputsAccess) {
    check_ast_roundtrip(R"(["$inputs", "/0"])");
}

TEST_F(SugarRoundtripTest, Addition) {
    check_ast_roundtrip(R"(["+", 1, 2])");
}

TEST_F(SugarRoundtripTest, VariadicAddition) {
    check_ast_roundtrip(R"(["+", 1, 2, 3])");
}

TEST_F(SugarRoundtripTest, Multiplication) {
    check_ast_roundtrip(R"(["*", 2, 3])");
}

TEST_F(SugarRoundtripTest, Comparison) {
    check_ast_roundtrip(R"([">", 5, 3])");
}

TEST_F(SugarRoundtripTest, LogicalAnd) {
    check_ast_roundtrip(R"(["and", true, false])");
}

TEST_F(SugarRoundtripTest, LogicalOr) {
    check_ast_roundtrip(R"(["or", true, false])");
}

TEST_F(SugarRoundtripTest, NotOperator) {
    check_ast_roundtrip(R"(["not", true])");
}

TEST_F(SugarRoundtripTest, UnaryMinus) {
    check_ast_roundtrip(R"(["-", 5])");
}

TEST_F(SugarRoundtripTest, SimpleLambda) {
    check_ast_roundtrip(R"(["lambda", ["x"], ["+", ["$", "/x"], 1]])");
}

TEST_F(SugarRoundtripTest, MultiParamLambda) {
    check_ast_roundtrip(R"(["lambda", ["a", "b"], ["+", ["$", "/a"], ["$", "/b"]]])");
}

TEST_F(SugarRoundtripTest, SimpleIf) {
    check_ast_roundtrip(R"(["if", true, 1, 0])");
}

TEST_F(SugarRoundtripTest, SimpleLet) {
    check_ast_roundtrip(R"(["let", [["x", 10]], ["$", "/x"]])");
}

TEST_F(SugarRoundtripTest, MultiLet) {
    check_ast_roundtrip(R"(["let", [["x", 10], ["y", 20]], ["+", ["$", "/x"], ["$", "/y"]]])");
}

TEST_F(SugarRoundtripTest, FunctionCall) {
    check_ast_roundtrip(R"(["count", ["$", "/arr"]])");
}

TEST_F(SugarRoundtripTest, MultiArgFunction) {
    check_ast_roundtrip(R"(["filter", ["$", "/arr"], ["lambda", ["x"], [">", ["$", "/x"], 0]]])");
}

TEST_F(SugarRoundtripTest, ArrayLiteral) {
    check_ast_roundtrip(R"({"array": [1, 2, 3]})");
}

TEST_F(SugarRoundtripTest, NestedPrecedence) {
    check_ast_roundtrip(R"(["+", ["*", 2, 3], 4])");
}

TEST_F(SugarRoundtripTest, PrecedenceNeedsParens) {
    check_ast_roundtrip(R"(["*", ["+", 1, 2], 3])");
}

// --- Sugar -> AST -> Sugar roundtrips ---

TEST_F(SugarRoundtripTest, SugarSimpleExpr) {
    check_sugar_roundtrip("1 + 2 * 3");
}

TEST_F(SugarRoundtripTest, SugarGroupedExpr) {
    check_sugar_roundtrip("(1 + 2) * 3");
}

TEST_F(SugarRoundtripTest, SugarLambda) {
    check_sugar_roundtrip("(x) => x + 1");
}

TEST_F(SugarRoundtripTest, SugarLet) {
    check_sugar_roundtrip("let x = 10 in x + 1");
}

TEST_F(SugarRoundtripTest, SugarIf) {
    check_sugar_roundtrip("if x > 0 then x else -x");
}

TEST_F(SugarRoundtripTest, SugarFunctionCall) {
    check_sugar_roundtrip("count(arr)");
}

TEST_F(SugarRoundtripTest, SugarFilterExample) {
    check_sugar_roundtrip("filter($input/users, (u) => count(u/orders) > 0)");
}

TEST_F(SugarRoundtripTest, SugarArrayLiteral) {
    check_sugar_roundtrip("[1, 2, 3]");
}

// --- Execution equivalence ---

TEST_F(SugarRoundtripTest, ExecutionEquivalenceArithmetic) {
    auto json_ast = jsom::parse_document(R"(["+", ["*", 3, 4], 2])");
    auto sugar_ast = SugarParser::parse("3 * 4 + 2", parse_opts);

    auto json_result = computo::execute(json_ast, {json(nullptr)});
    auto sugar_result = computo::execute(sugar_ast, {json(nullptr)});
    EXPECT_EQ(json_result, sugar_result);
    EXPECT_EQ(json_result, json(14));
}

TEST_F(SugarRoundtripTest, ExecutionEquivalenceLet) {
    auto json_ast = jsom::parse_document(
        R"(["let", [["x", 10], ["y", 20]], ["+", ["$", "/x"], ["$", "/y"]]])"
    );
    auto sugar_ast = SugarParser::parse("let x = 10, y = 20 in x + y", parse_opts);

    auto json_result = computo::execute(json_ast, {json(nullptr)});
    auto sugar_result = computo::execute(sugar_ast, {json(nullptr)});
    EXPECT_EQ(json_result, sugar_result);
    EXPECT_EQ(json_result, json(30));
}

TEST_F(SugarRoundtripTest, ExecutionEquivalenceIf) {
    auto json_ast = jsom::parse_document(R"(["if", [">", 5, 3], "yes", "no"])");
    auto sugar_ast = SugarParser::parse("if 5 > 3 then \"yes\" else \"no\"", parse_opts);

    auto json_result = computo::execute(json_ast, {json(nullptr)});
    auto sugar_result = computo::execute(sugar_ast, {json(nullptr)});
    EXPECT_EQ(json_result, sugar_result);
    EXPECT_EQ(json_result, json("yes"));
}

TEST_F(SugarRoundtripTest, ExecutionEquivalenceLambdaMap) {
    auto json_ast = jsom::parse_document(
        R"(["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])"
    );
    auto sugar_ast = SugarParser::parse("map([1, 2, 3], (x) => x * 2)", parse_opts);

    auto json_result = computo::execute(json_ast, {json(nullptr)});
    auto sugar_result = computo::execute(sugar_ast, {json(nullptr)});
    EXPECT_EQ(json_result, sugar_result);
}

TEST_F(SugarRoundtripTest, ExecutionEquivalenceFilter) {
    auto json_ast = jsom::parse_document(
        R"(["filter", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 3]]])"
    );
    auto sugar_ast = SugarParser::parse("filter([1, 2, 3, 4, 5], (x) => x > 3)", parse_opts);

    auto json_result = computo::execute(json_ast, {json(nullptr)});
    auto sugar_result = computo::execute(sugar_ast, {json(nullptr)});
    EXPECT_EQ(json_result, sugar_result);
}

// --- Complex motivating example ---

TEST_F(SugarRoundtripTest, MotivatingExampleRoundtrip) {
    std::string sugar = R"(
        let
          active = filter($input/users, (u) => count(u/orders) > 0)
        in
          active
    )";
    auto ast = SugarParser::parse(sugar, parse_opts);
    EXPECT_TRUE(ast.is_array());
    EXPECT_EQ(ast[0].as<std::string>(), "let");

    // Write back to sugar and re-parse
    auto written = SugarWriter::write(ast, write_opts);
    auto reparsed = SugarParser::parse(written, parse_opts);
    EXPECT_EQ(reparsed, ast);
}
