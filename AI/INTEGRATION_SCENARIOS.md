# Integration Test Scenarios

## Purpose

This document defines comprehensive integration test scenarios that validate Computo's behavior in realistic, complex situations. These scenarios go beyond unit tests to ensure that operator interactions, error handling, and performance characteristics work correctly in real-world usage patterns.

## Testing Philosophy

### 1. **Real-World Relevance**
- Test scenarios that mirror actual production use cases
- Include realistic data sizes and complexity levels
- Cover edge cases that occur in practice, not just theoretical extremes

### 2. **End-to-End Validation**
- Test complete transformation pipelines, not just individual operators
- Validate error propagation through complex expressions
- Ensure performance characteristics hold under realistic load

### 3. **Cross-Cutting Concerns**
- Test operator interactions and combinations
- Validate thread safety under concurrent load
- Ensure memory management works correctly across operation boundaries

## Core Integration Scenarios

### Scenario 1: API Gateway JSON Transformation

#### Use Case
Transform incoming API requests between different service formats (e.g., REST to GraphQL, vendor API format conversion).

#### Test Data
```json
// Input: Legacy API format
{
  "user_data": {
    "id": "12345",
    "name": "Alice Johnson",
    "email": "alice@example.com",
    "created": "2023-01-15T10:30:00Z",
    "preferences": {
      "notifications": true,
      "theme": "dark",
      "language": "en"
    },
    "roles": ["user", "premium"]
  },
  "metadata": {
    "version": "1.0",
    "timestamp": 1673778600
  }
}
```

#### Transformation Script
```json
["let", [
  ["user", ["$input", "/user_data"]],
  ["meta", ["$input", "/metadata"]]
], [
  "obj",
  ["id", ["$", "/user/id"]],
  ["profile", [
    "obj",
    ["fullName", ["$", "/user/name"]],
    ["email", ["$", "/user/email"]],
    ["joinDate", ["$", "/user/created"]],
    ["settings", ["$", "/user/preferences"]]
  ]],
  ["permissions", [
    "obj",
    ["roles", ["$", "/user/roles"]],
    ["isPremium", ["some", ["$", "/user/roles"], 
      ["lambda", ["role"], ["==", ["$", "/role"], "premium"]]
    ]]
  ]],
  ["apiVersion", ["$", "/meta/version"]]
]]
```

#### Expected Output
```json
{
  "id": "12345",
  "profile": {
    "fullName": "Alice Johnson",
    "email": "alice@example.com", 
    "joinDate": "2023-01-15T10:30:00Z",
    "settings": {
      "notifications": true,
      "theme": "dark",
      "language": "en"
    }
  },
  "permissions": {
    "roles": ["user", "premium"],
    "isPremium": true
  },
  "apiVersion": "1.0"
}
```

#### Validation Points
- **Functional**: Output matches expected structure exactly
- **Performance**: Transformation completes in < 10ms
- **Memory**: Peak memory usage < 3x input size
- **Error Handling**: Graceful handling of missing fields with defaults

#### Error Scenarios
```json
// Missing user_data
{"metadata": {"version": "1.0"}}
// Expected: Specific error about missing /user_data path

// Invalid date format  
{"user_data": {"created": "invalid-date"}}
// Expected: Pass through invalid data (no validation in transformation)

// Missing nested preferences
{"user_data": {"id": "123", "name": "Alice"}}
// Expected: Handle missing /user/preferences gracefully
```

### Scenario 2: Data Processing Pipeline

#### Use Case
Process large arrays of user data for analytics, including filtering, aggregation, and statistical analysis.

#### Test Data
```json
{
  "users": [
    {"id": 1, "name": "Alice", "age": 28, "department": "engineering", "salary": 85000, "performance": 4.2},
    {"id": 2, "name": "Bob", "age": 32, "department": "sales", "salary": 72000, "performance": 3.8},
    {"id": 3, "name": "Charlie", "age": 29, "department": "engineering", "salary": 90000, "performance": 4.5},
    {"id": 4, "name": "Diana", "age": 35, "department": "marketing", "salary": 78000, "performance": 4.0},
    // ... 1000 total users for performance testing
  ]
}
```

#### Transformation Script
```json
["let", [
  ["all_users", ["$input", "/users"]],
  ["high_performers", ["filter", ["$", "/all_users"],
    ["lambda", ["user"], [">", ["$", "/user/performance"], 4.0]]
  ]],
  ["by_department", ["reduce", ["$", "/high_performers"],
    ["lambda", ["acc", "user"], 
      ["let", [["dept", ["$", "/user/department"]]],
        ["obj", 
          [["$", "/dept"], 
            ["+", ["$", "/acc", ["strConcat", "/", ["$", "/dept"]], 0], 1]
          ]
        ]
      ]
    ], {}
  ]]
], [
  "obj",
  ["total_high_performers", ["count", ["$", "/high_performers"]]],
  ["average_salary", ["reduce", ["$", "/high_performers"],
    ["lambda", ["acc", "user"], ["+", ["$", "/acc"], ["$", "/user/salary"]]],
    0
  ]],
  ["department_breakdown", ["$", "/by_department"]],
  ["top_performer", ["car", ["sort", ["$", "/high_performers"], 
    ["/performance", "desc"]
  ]]]
]]
```

#### Expected Behavior
- **Filtering**: Correctly identify users with performance > 4.0
- **Aggregation**: Calculate correct totals and averages
- **Grouping**: Properly group by department
- **Sorting**: Find actual top performer

#### Performance Requirements
- **Small Dataset** (< 100 users): < 50ms
- **Medium Dataset** (1000 users): < 500ms  
- **Large Dataset** (10000 users): < 5 seconds
- **Memory Usage**: < 2x input size peak

#### Stress Test Variations
```json
// Deep nesting stress test
["map", ["$input", "/users"],
  ["lambda", ["user"],
    ["let", [["enriched", 
      ["obj", 
        ["original", ["$", "/user"]],
        ["calculated", [
          "obj",
          ["tax_bracket", ["if", [">", ["$", "/user/salary"], 80000], "high", "standard"]],
          ["seniority", ["if", [">", ["$", "/user/age"], 30], "senior", "junior"]],
          ["bonus_eligible", ["&&", 
            [">", ["$", "/user/performance"], 3.5],
            ["==", ["$", "/user/tax_bracket"], "high"]
          ]]
        ]]
      ]
    ]], ["$", "/enriched"]
  ]
]]
```

### Scenario 3: Configuration Processing

#### Use Case
Process complex application configuration with environment-specific overrides, validation, and default value application.

#### Test Data
```json
{
  "base_config": {
    "database": {
      "host": "localhost",
      "port": 5432,
      "name": "myapp"
    },
    "cache": {
      "type": "redis",
      "ttl": 3600
    },
    "features": {
      "new_ui": false,
      "analytics": true
    }
  },
  "environment": "production",
  "overrides": {
    "database": {
      "host": "prod-db.example.com",
      "ssl": true
    },
    "cache": {
      "ttl": 7200
    },
    "features": {
      "new_ui": true
    }
  }
}
```

#### Transformation Script
```json
["let", [
  ["base", ["$input", "/base_config"]],
  ["env", ["$input", "/environment"]],
  ["overrides", ["$input", "/overrides", {}]],
  ["apply_overrides", ["lambda", ["base_obj", "override_obj"],
    ["reduce", ["keys", ["$", "/override_obj"]],
      ["lambda", ["acc", "key"],
        ["obj", 
          [["$", "/key"], 
            ["if", ["==", ["type", ["$", "/base_obj", ["strConcat", "/", ["$", "/key"]]]], "object"],
              ["apply_overrides", 
                ["$", "/base_obj", ["strConcat", "/", ["$", "/key"]]],
                ["$", "/override_obj", ["strConcat", "/", ["$", "/key"]]]
              ],
              ["$", "/override_obj", ["strConcat", "/", ["$", "/key"]]]
            ]
          ]
        ]
      ],
      ["$", "/base_obj"]
    ]
  ]]
], [
  "obj",
  ["environment", ["$", "/env"]],
  ["config", ["apply_overrides", ["$", "/base"], ["$", "/overrides"]]],
  ["validation", [
    "obj",
    ["required_fields_present", ["every", 
      {"array": ["/config/database/host", "/config/database/port", "/config/cache/type"]},
      ["lambda", ["path"], ["!=", ["$input", ["$", "/path"]], null]]
    ]],
    ["valid_environment", ["some", 
      {"array": ["development", "staging", "production"]},
      ["lambda", ["valid_env"], ["==", ["$", "/env"], ["$", "/valid_env"]]]
    ]]
  ]]
]]
```

#### Validation Points
- **Deep Merging**: Nested objects merge correctly without overwriting siblings
- **Type Preservation**: Override values maintain correct types
- **Validation Logic**: Configuration validation catches common errors
- **Default Handling**: Missing values handled gracefully

#### Error Scenarios
```json
// Invalid environment
{"base_config": {...}, "environment": "invalid", "overrides": {}}

// Missing required configuration
{"base_config": {"cache": {"type": "redis"}}, "environment": "production"}

// Type conflicts in overrides
{"base_config": {"port": 5432}, "overrides": {"port": "not_a_number"}}
```

### Scenario 4: Multi-Input Data Correlation

#### Use Case
Correlate data from multiple sources (e.g., user data, transaction data, analytics data) to generate comprehensive reports.

#### Test Data (Multiple Inputs)
```json
// Input 1: Users
{"users": [
  {"id": 1, "name": "Alice", "email": "alice@example.com", "signup_date": "2023-01-15"},
  {"id": 2, "name": "Bob", "email": "bob@example.com", "signup_date": "2023-02-10"}
]}

// Input 2: Transactions  
{"transactions": [
  {"user_id": 1, "amount": 99.99, "date": "2023-03-01", "product": "premium"},
  {"user_id": 1, "amount": 29.99, "date": "2023-03-15", "product": "addon"},
  {"user_id": 2, "amount": 49.99, "date": "2023-03-20", "product": "basic"}
]}

// Input 3: Analytics
{"analytics": [
  {"user_id": 1, "page_views": 145, "session_duration": 1200, "last_login": "2023-03-22"},
  {"user_id": 2, "page_views": 67, "session_duration": 800, "last_login": "2023-03-21"}
]}
```

#### Transformation Script
```json
["let", [
  ["users", ["$inputs", "/0/users"]],
  ["transactions", ["$inputs", "/1/transactions"]],
  ["analytics", ["$inputs", "/2/analytics"]]
], [
  "map", ["$", "/users"],
  ["lambda", ["user"],
    ["let", [
      ["user_id", ["$", "/user/id"]],
      ["user_transactions", ["filter", ["$", "/transactions"],
        ["lambda", ["tx"], ["==", ["$", "/tx/user_id"], ["$", "/user_id"]]]
      ]],
      ["user_analytics", ["find", ["$", "/analytics"],
        ["lambda", ["analytics"], ["==", ["$", "/analytics/user_id"], ["$", "/user_id"]]]
      ]]
    ], [
      "obj",
      ["user_info", ["$", "/user"]],
      ["transaction_summary", [
        "obj",
        ["total_spent", ["reduce", ["$", "/user_transactions"],
          ["lambda", ["acc", "tx"], ["+", ["$", "/acc"], ["$", "/tx/amount"]]],
          0
        ]],
        ["transaction_count", ["count", ["$", "/user_transactions"]]],
        ["latest_transaction", ["car", ["sort", ["$", "/user_transactions"], 
          ["/date", "desc"]
        ]]]
      ]],
      ["engagement", ["$", "/user_analytics", {}]]
    ]]
  ]
]]
```

#### Validation Points
- **Multi-Input Access**: Correctly access different input sources
- **Data Correlation**: Properly join data across sources by ID
- **Null Handling**: Gracefully handle missing correlations
- **Performance**: Efficient even with large datasets

### Scenario 5: Error Propagation and Recovery

#### Use Case
Test how errors propagate through complex nested expressions and ensure users can debug issues effectively.

#### Test Data with Intentional Issues
```json
{
  "users": [
    {"id": 1, "name": "Alice", "age": 28, "salary": 85000},
    {"id": 2, "name": "Bob", "age": "invalid", "salary": null},
    {"id": 3, "name": null, "age": 32, "salary": 75000}
  ]
}
```

#### Transformation Scripts with Errors
```json
// Test 1: Unknown operator deep in expression
["map", ["$input", "/users"],
  ["lambda", ["user"], 
    ["obj", 
      ["processed_name", ["upcase", ["$", "/user/name"]]], // "upcase" doesn't exist
      ["age_category", ["if", [">", ["$", "/user/age"], 30], "senior", "junior"]]
    ]
  ]
]

// Test 2: Type error in nested calculation
["let", [["total_salary", 
  ["reduce", ["$input", "/users"],
    ["lambda", ["acc", "user"], 
      ["+", ["$", "/acc"], ["$", "/user/salary"]] // Will fail on null salary
    ], 0]
]], ["$", "/total_salary"]]

// Test 3: Invalid path access
["map", ["$input", "/users"],
  ["lambda", ["user"],
    ["$", "/user/missing/deeply/nested/field"]
  ]
]
```

#### Expected Error Behaviors
- **Precise Location**: Errors report exact path where failure occurred
- **Context Preservation**: Error messages include enough context to debug
- **Helpful Suggestions**: Unknown operators suggest correct alternatives
- **Type Information**: Type errors explain expected vs actual types

#### Error Message Quality Tests
```
Error at /map/lambda/body/obj/processed_name: Unknown operator "upcase". Did you mean "strConcat"?

Error at /let/total_salary/reduce/lambda/body: Cannot add number and null at argument 2. Consider using default values or filtering null values.

Error at /map/lambda/body: Path "/user/missing/deeply/nested/field" not found. Available fields: id, name, age, salary.
```

## Cross-Platform Integration Tests

### Platform-Specific Scenarios

#### Thread Safety Under Load
```cpp
// Concurrent execution test
std::vector<std::thread> threads;
std::atomic<int> success_count{0};
std::atomic<int> error_count{0};

for (int i = 0; i < 100; ++i) {
    threads.emplace_back([&, i]() {
        try {
            auto script = nlohmann::json::array({"+", i, i, i});
            auto result = computo::execute(script);
            success_count++;
        } catch (...) {
            error_count++;
        }
    });
}
```

#### Memory Management Stress
```cpp
// Memory stress test
for (int i = 0; i < 10000; ++i) {
    auto large_array = create_large_test_array(1000); // 1000 items
    auto script = nlohmann::json::array({"map", large_array, simple_lambda});
    auto result = computo::execute(script);
    // Verify memory is released between iterations
}
```

#### File I/O Integration
```bash
# CLI integration tests
echo '["$input", "/data"]' > transform.json
echo '{"data": [1, 2, 3]}' > input.json
./computo transform.json input.json > output.json
# Verify output.json contains [1, 2, 3]
```

## Performance Integration Tests

### Load Testing Scenarios

#### Sustained Throughput Test
- Execute 10,000 transformations over 60 seconds
- Verify consistent response times throughout test
- Ensure no memory leaks during sustained operation
- Validate thread pool efficiency

#### Burst Load Test  
- Execute 1,000 transformations as fast as possible
- Verify system handles burst without failure
- Ensure performance returns to baseline after burst
- Validate resource cleanup

#### Memory Pressure Test
- Execute transformations with increasingly large data sets
- Monitor memory usage patterns and peak consumption
- Verify graceful degradation under memory pressure
- Ensure system remains responsive

## Edge Case Integration Tests

### Boundary Condition Tests

#### Very Large Data
```json
// 1MB+ JSON input with complex transformations
{
  "large_array": [/* 100,000 objects */],
  "deep_nesting": {/* 50 levels deep */}
}
```

#### Very Deep Nesting
```json
// Expression nesting beyond typical usage
["+", ["+", ["+", ["+", ["+", /* 100 levels deep */]]]]]
```

#### Unicode and Special Characters
```json
{
  "unicode_data": "Hello 世界   émojis",
  "special_chars": "quotes\"backslash\\tab\tnewline\n"
}
```

### Malformed Input Tests

#### Invalid JSON Structure
- Incomplete JSON objects
- Mismatched brackets and braces
- Invalid escape sequences
- Non-UTF8 byte sequences

#### Schema Violations
- Arrays where objects expected
- Null values in unexpected places
- Mixed type arrays
- Circular references (if supported)

## Continuous Integration Test Suite

### Automated Test Execution

#### Test Categories
1. **Smoke Tests**: Basic functionality across all operators
2. **Integration Tests**: All scenarios in this document
3. **Performance Tests**: Benchmark validation with regression detection
4. **Stress Tests**: Resource exhaustion and recovery
5. **Platform Tests**: Cross-platform behavior validation

#### CI Pipeline Requirements
```yaml
integration_tests:
  runs-on: [ubuntu-latest, windows-latest, macos-latest]
  steps:
    - name: Build Computo
    - name: Unit Tests
    - name: Integration Tests
    - name: Performance Benchmarks
    - name: Memory Leak Detection
    - name: Thread Safety Tests
```

#### Success Criteria
- All functional tests pass on all platforms
- Performance remains within 10% of baseline
- No memory leaks detected
- Thread safety tests complete without race conditions
- Error message quality meets standards

## Test Data Management

### Realistic Test Data Sets

#### Small Scale (Development)
- 10-100 records for quick iteration
- Covers all common data types and structures
- Includes representative edge cases

#### Medium Scale (CI)
- 1,000-10,000 records for performance validation
- Real-world data complexity and variety
- Multiple scenarios for comprehensive coverage

#### Large Scale (Stress Testing)
- 100,000+ records for scalability testing
- Production-representative data sizes
- Performance regression detection

### Test Data Generation
```python
# Generate realistic test data for different scenarios
def generate_user_data(count):
    return [
        {
            "id": i,
            "name": fake.name(),
            "email": fake.email(),
            "age": random.randint(18, 65),
            "department": random.choice(["engineering", "sales", "marketing"]),
            "salary": random.randint(40000, 150000),
            "performance": round(random.uniform(2.0, 5.0), 1)
        }
        for i in range(count)
    ]
```

## Conclusion

These integration test scenarios ensure that Computo works correctly in realistic production situations, not just in isolated unit tests. They validate that:

1. **Complex transformations** work correctly end-to-end
2. **Error handling** provides useful debugging information
3. **Performance** meets requirements under realistic load
4. **Memory management** works correctly across operation boundaries
5. **Thread safety** is maintained under concurrent usage
6. **Cross-platform** behavior is consistent

Any rewrite of Computo should pass all these integration tests before being considered complete. The test scenarios should be executable as part of an automated test suite and should be expanded as new use cases are discovered in production usage.
