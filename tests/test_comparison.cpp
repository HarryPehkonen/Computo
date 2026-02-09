#include "computo.hpp"
#include <gtest/gtest.h>

using json = jsom::JsonDocument;

TEST(ComparisonOperators, GreaterThanBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">", 5, 3])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">", 3, 5])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">", 5, 5])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">", 5.5, 3.3])"), {json(nullptr)}), true);
}

TEST(ComparisonOperators, GreaterThanChaining) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">", 10, 5, 3])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">", 10, 3, 5])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">", 10, 5, 5])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">", 10, 8, 6, 4])"), {json(nullptr)}), true);
}

TEST(ComparisonOperators, GreaterThanErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"([">"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"([">", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"([">", "not_a_number", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(ComparisonOperators, LessThanBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<", 3, 5])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<", 5, 3])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<", 5, 5])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<", 3.3, 5.5])"), {json(nullptr)}), true);
}

TEST(ComparisonOperators, LessThanChaining) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<", 3, 5, 10])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<", 5, 3, 10])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<", 3, 5, 5])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<", 1, 3, 5, 7])"), {json(nullptr)}), true);
}

TEST(ComparisonOperators, LessThanErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["<"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["<", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["<", "not_a_number", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(ComparisonOperators, GreaterEqualBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">=", 5, 3])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">=", 5, 5])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">=", 3, 5])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">=", 5.5, 3.3])"), {json(nullptr)}), true);
}

TEST(ComparisonOperators, GreaterEqualChaining) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">=", 10, 5, 3])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">=", 10, 10, 5])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">=", 10, 5, 8])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"([">=", 10, 8, 6, 4])"), {json(nullptr)}), true);
}

TEST(ComparisonOperators, GreaterEqualErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"([">="])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"([">=", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"([">=", "not_a_number", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(ComparisonOperators, LessEqualBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<=", 3, 5])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<=", 5, 5])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<=", 5, 3])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<=", 3.3, 5.5])"), {json(nullptr)}), true);
}

TEST(ComparisonOperators, LessEqualChaining) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<=", 3, 5, 10])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<=", 3, 3, 10])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<=", 3, 5, 4])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["<=", 1, 3, 5, 7])"), {json(nullptr)}), true);
}

TEST(ComparisonOperators, LessEqualErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["<="])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["<=", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["<=", "not_a_number", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(ComparisonOperators, EqualBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["==", 5, 5])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["==", 5, 3])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["==", "hello", "hello"])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["==", "hello", "world"])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["==", true, true])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["==", true, false])"), {json(nullptr)}), false);
}

TEST(ComparisonOperators, EqualNary) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["==", 5, 5, 5])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["==", 5, 5, 3])"), {json(nullptr)}), false);
    EXPECT_EQ(
        computo::execute(jsom::parse_document(R"(["==", "hello", "hello", "hello"])"), {json(nullptr)}),
        true);
    EXPECT_EQ(
        computo::execute(jsom::parse_document(R"(["==", "hello", "hello", "world"])"), {json(nullptr)}),
        false);
}

TEST(ComparisonOperators, EqualErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["=="])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["==", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(ComparisonOperators, NotEqualBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["!=", 5, 3])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["!=", 5, 5])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["!=", "hello", "world"])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["!=", "hello", "hello"])"), {json(nullptr)}), false);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["!=", true, false])"), {json(nullptr)}), true);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["!=", true, true])"), {json(nullptr)}), false);
}

TEST(ComparisonOperators, NotEqualErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["!="])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["!=", 5])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["!=", 5, 3, 7])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}
