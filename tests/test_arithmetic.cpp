#include "computo.hpp"
#include <gtest/gtest.h>

using json = jsom::JsonDocument;

TEST(ArithmeticOperators, AdditionBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["+", 1, 2])"), {json(nullptr)}), 3);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["+", 1.5, 2.5])"), {json(nullptr)}), 4.0);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["+", 1, 2.5])"), {json(nullptr)}), 3.5);
}

TEST(ArithmeticOperators, AdditionNary) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["+", 1, 2, 3, 4])"), {json(nullptr)}), 10);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["+", 1.1, 2.2, 3.3])"), {json(nullptr)}), 6.6);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["+", 42])"), {json(nullptr)}), 42);
}

TEST(ArithmeticOperators, AdditionErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["+"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["+", "not_a_number"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["+", 1, "not_a_number"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(ArithmeticOperators, SubtractionBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["-", 5, 3])"), {json(nullptr)}), 2);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["-", 5.5, 2.5])"), {json(nullptr)}), 3.0);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["-", 42])"), {json(nullptr)}), -42);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["-", -10])"), {json(nullptr)}), 10);
}

TEST(ArithmeticOperators, SubtractionNary) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["-", 10, 2, 3])"), {json(nullptr)}), 5);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["-", 20.5, 5.5, 10])"), {json(nullptr)}), 5.0);
}

TEST(ArithmeticOperators, SubtractionErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["-"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["-", "not_a_number"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(ArithmeticOperators, MultiplicationBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["*", 3, 4])"), {json(nullptr)}), 12);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["*", 2.5, 4])"), {json(nullptr)}), 10.0);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["*", 42])"), {json(nullptr)}), 42);
}

TEST(ArithmeticOperators, MultiplicationNary) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["*", 2, 3, 4])"), {json(nullptr)}), 24);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["*", 1.5, 2, 3])"), {json(nullptr)}), 9.0);
}

TEST(ArithmeticOperators, MultiplicationErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["*"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["*", "not_a_number"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(ArithmeticOperators, DivisionBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["/", 12, 3])"), {json(nullptr)}), 4);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["/", 10, 4])"), {json(nullptr)}), 2.5);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["/", 4])"), {json(nullptr)}), 0.25);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["/", 0.5])"), {json(nullptr)}), 2.0);
}

TEST(ArithmeticOperators, DivisionNary) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["/", 24, 2, 3])"), {json(nullptr)}), 4);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["/", 100, 2, 5])"), {json(nullptr)}), 10.0);
}

TEST(ArithmeticOperators, DivisionErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["/"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["/", 0])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["/", 10, 0])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["/", "not_a_number"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(ArithmeticOperators, ModuloBasic) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["%", 10, 3])"), {json(nullptr)}), 1);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["%", 15, 4])"), {json(nullptr)}), 3);
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["%", 7.5, 2.5])"), {json(nullptr)}), 0.0);
}

TEST(ArithmeticOperators, ModuloNary) {
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["%", 25, 7, 3])"), {json(nullptr)}), 1);
}

TEST(ArithmeticOperators, ModuloErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["%"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["%", 10])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["%", 10, 0])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["%", "not_a_number"])"), {json(nullptr)}),
                 computo::InvalidArgumentException);
}
