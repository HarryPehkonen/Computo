#include <chrono>
#include <computo.hpp>
#include <future>
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <vector>

using json = nlohmann::json;

class ThreadSafetyTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(ThreadSafetyTest, ConcurrentSimpleExecution) {
    const int num_threads = 10;
    const int iterations = 100;

    auto worker = [this](int) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);

        for (int i = 0; i < iterations; ++i) {
            int a = dis(gen);
            int b = dis(gen);
            json script = json::array({ "+", a, b });
            auto result = exec(script);
            EXPECT_EQ(result, a + b);
        }
    };

    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_threads; ++i) {
        futures.push_back(std::async(std::launch::async, worker, i));
    }

    for (auto& future : futures) {
        future.get();
    }
}

TEST_F(ThreadSafetyTest, ConcurrentArrayOperations) {
    const int num_threads = 5;
    const int iterations = 50;

    auto worker = [this](int) {
        for (int i = 0; i < iterations; ++i) {
            json script = json::array({ "map",
                json::object({ { "array", json::array({ 1, 2, 3, 4, 5 }) } }),
                json::array({ "lambda", json::array({ "x" }), json::array({ "*", json::array({ "$", "/x" }), 2 }) }) });
            auto result = exec(script);
            json expected = R"([2, 4, 6, 8, 10])"_json;  // Clean array output
            EXPECT_EQ(result, expected);
        }
    };

    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_threads; ++i) {
        futures.push_back(std::async(std::launch::async, worker, i));
    }

    for (auto& future : futures) {
        future.get();
    }
}

TEST_F(ThreadSafetyTest, ConcurrentComplexExpressions) {
    const int num_threads = 3;
    const int iterations = 20;

    auto worker = [this](int) {
        for (int i = 0; i < iterations; ++i) {
            json script = json::array({ "let",
                json::array({ json::array({ "x", 10 }),
                    json::array({ "y", 20 }) }),
                json::array({ "let",
                    json::array({ json::array({ "z", json::array({ "+", json::array({ "$", "/x" }), json::array({ "$", "/y" }) }) }) }),
                    json::array({ "map",
                        json::object({ { "array", json::array({ 1, 2, 3 }) } }),
                        json::array({ "lambda", json::array({ "n" }), json::array({ "*", json::array({ "$", "/n" }), json::array({ "$", "/z" }) }) }) }) }) });
            auto result = exec(script);
            json expected = R"([30, 60, 90])"_json;  // Clean array output
            EXPECT_EQ(result, expected);
        }
    };

    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_threads; ++i) {
        futures.push_back(std::async(std::launch::async, worker, i));
    }

    for (auto& future : futures) {
        future.get();
    }
}

TEST_F(ThreadSafetyTest, StressTest) {
    const int num_threads = 20;
    const int iterations = 1000;

    auto worker = [this](int) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 10);

        for (int i = 0; i < iterations; ++i) {
            int op = dis(gen);
            json script;

            switch (op) {
            case 1:
                script = json::array({ "+", 1, 2 });
                EXPECT_EQ(exec(script), 3);
                break;
            case 2:
                script = json::array({ "*", 3, 4 });
                EXPECT_EQ(exec(script), 12);
                break;
            case 3:
                script = json::array({ ">", 5, 3 });
                EXPECT_EQ(exec(script), true);
                break;
            case 4:
                script = json::array({ "count", json::object({ { "array", json::array({ 1, 2, 3 }) } }) });
                EXPECT_EQ(exec(script), 3);
                break;
            case 5:
                script = json::array({ "car", json::object({ { "array", json::array({ 1, 2, 3 }) } }) });
                EXPECT_EQ(exec(script), 1);
                break;
            default:
                script = json::array({ "&&", true, false });
                EXPECT_EQ(exec(script), false);
                break;
            }
        }
    };

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_threads; ++i) {
        futures.push_back(std::async(std::launch::async, worker, i));
    }

    for (auto& future : futures) {
        future.get();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Stress test completed: " << num_threads * iterations
              << " operations in " << duration.count() << "ms" << std::endl;
}