# TabTools-Inspired Ideas for Computo

This document explores how patterns from the tabTools CLI suite could be adapted for Computo's JSON-Lisp transformation engine, bringing SQL-like data manipulation capabilities to JSON processing.

## Background

TabTools was a suite of CLI tools for manipulating text-based tables (CSV, tab-delimited) with operations like sorting, filtering, joining, and pivoting. The core insight is that many of these table operations could be powerful when applied to JSON data structures.

## Core Concepts

### 1. Multi-Key Sorting (à la `ttSort`)

Instead of simple array sorting, enable sophisticated nested property sorting:

```json
["sort-by", 
  ["$", "/users"],
  [["$", "/lastName"], ["$", "/firstName"], ["$", "/age"]]
]
```

This would sort a JSON array by multiple nested properties with proper precedence handling.

### 2. Pivot Operations (à la `ttBuild`) - The Game Changer

Transform flat arrays into cross-tabulated structures:

**Input JSON:**
```json
{
  "sales": [
    {"region": "North", "month": "Jan", "product": "A", "revenue": 1000},
    {"region": "North", "month": "Jan", "product": "B", "revenue": 1500},
    {"region": "South", "month": "Jan", "product": "A", "revenue": 800},
    {"region": "North", "month": "Feb", "product": "A", "revenue": 1200}
  ]
}
```

**Computo Pivot Operation:**
```json
["pivot",
  ["$", "/sales"],
  {
    "rows": ["$", "/region"],
    "columns": ["$", "/month"], 
    "values": ["$", "/revenue"],
    "aggregate": "sum"
  }
]
```

**Result:**
```json
{
  "North": {"Jan": 2500, "Feb": 1200},
  "South": {"Jan": 800, "Feb": 0}
}
```

### 3. Group-By Aggregation (à la `ttCount`)

Powerful grouping with multiple aggregation functions:

```json
["group-by",
  ["$", "/transactions"],
  ["$", "/category"],
  {
    "count": ["count"],
    "total": ["sum", ["$", "/amount"]],
    "avg": ["mean", ["$", "/amount"]],
    "min": ["min", ["$", "/amount"]],
    "max": ["max", ["$", "/amount"]]
  }
]
```

### 4. Join Operations (à la `ttMerge`)

Relational joins between JSON data sources:

```json
["join",
  {
    "left": ["$", "/orders"], 
    "right": ["$1", "/customers"],
    "on": [["$", "/customerId"], ["$", "/id"]],
    "type": "inner"
  }
]
```

**Join Types:**
- `inner` - Only matching records
- `left` - All left records, matching right records
- `right` - All right records, matching left records  
- `outer` - All records from both sides

### 5. Sequence Building (à la `ttSeq`)

Event sequence aggregation and path building:

```json
["sequence-by",
  ["$", "/events"],
  ["$", "/userId"],
  {
    "path": ["join", ["$", "/action"], " → "],
    "duration": ["sum", ["$", "/timeSpent"]],
    "count": ["count"]
  }
]
```

### 6. Advanced Filtering (à la `ttMatch`)

Complex predicate-based filtering:

```json
["filter",
  ["$", "/products"],
  ["and",
    [">=", ["$", "/price"], 100],
    ["in", ["$", "/category"], ["electronics", "computers"]],
    ["matches", ["$", "/name"], "^Apple.*"],
    ["not", ["eq", ["$", "/status"], "discontinued"]]
  ]
]
```

## Proposed "Table" Operator Family

A comprehensive set of table-like operations for JSON:

### Core Operations

| Operator | Purpose | TabTools Equivalent |
|----------|---------|-------------------|
| `table-sort` | Multi-key sorting | `ttSort` |
| `table-group` | Grouping with aggregation | `ttCount` |
| `table-join` | Relational joins | `ttMerge`, `ttMap` |
| `table-pivot` | Cross-tabulation | `ttBuild` |
| `table-filter` | Complex filtering | `ttMatch`, `ttAvoid` |
| `table-select` | Column selection | `ttSelect` |
| `table-sequence` | Event sequencing | `ttSeq` |

### Advanced Operations

| Operator | Purpose | Description |
|----------|---------|-------------|
| `table-transpose` | Flip rows/columns | Matrix transposition |
| `table-union` | Combine datasets | Set union with deduplication |
| `table-intersect` | Common records | Set intersection |
| `table-diff` | Record differences | Set difference |
| `table-sample` | Random sampling | Statistical sampling |

## Real-World Example: Sales Analysis Pipeline

```json
["let",
  [
    ["filtered_sales", 
     ["table-filter", 
      ["$", "/sales"], 
      [">=", ["$", "/date"], "2024-01-01"]]],
    
    ["grouped_sales",
     ["table-group",
      ["$", "/filtered_sales"],
      ["$", "/region"],
      {
        "total_revenue": ["sum", ["$", "/revenue"]],
        "avg_deal_size": ["mean", ["$", "/revenue"]],
        "deal_count": ["count"],
        "largest_deal": ["max", ["$", "/revenue"]]
      }]],
    
    ["pivoted_sales",
     ["table-pivot",
      ["$", "/filtered_sales"],
      {
        "rows": ["$", "/region"],
        "columns": ["$", "/quarter"], 
        "values": ["$", "/revenue"],
        "aggregate": "sum"
      }]],
    
    ["top_customers",
     ["table-sort",
      ["table-group",
       ["$", "/filtered_sales"],
       ["$", "/customerId"],
       {"revenue": ["sum", ["$", "/revenue"]]}],
      [["$", "/revenue"]]]]
  ],
  {
    "summary": ["$", "/grouped_sales"],
    "pivot_table": ["$", "/pivoted_sales"],
    "top_10_customers": ["take", 10, ["$", "/top_customers"]]
  }
]
```

## Advanced Use Cases

### 1. Data Warehousing Operations

```json
["table-join",
  {
    "left": ["table-group", ["$", "/transactions"], ["$", "/date"], {...}],
    "right": ["table-group", ["$1", "/customer_events"], ["$", "/date"], {...}],
    "on": [["$", "/date"], ["$", "/date"]],
    "type": "outer"
  }
]
```

### 2. Time Series Analysis

```json
["table-sequence",
  ["$", "/user_events"],
  ["$", "/sessionId"],
  {
    "funnel": ["join", ["$", "/step"], " → "],
    "conversion_time": ["sum", ["$", "/duration"]],
    "drop_off_point": ["last", ["$", "/step"]]
  }
]
```

### 3. A/B Testing Analysis

```json
["table-pivot",
  ["table-filter", ["$", "/experiments"], ["in", ["$", "/variant"], ["A", "B"]]],
  {
    "rows": ["$", "/variant"],
    "columns": ["$", "/metric"],
    "values": ["$", "/value"],
    "aggregate": "mean"
  }
]
```

## Implementation Strategy

### Decomposition Approach

These table operations can be implemented as **high-level operators** that decompose into existing Computo primitives:

- `table-sort` → complex nested `sort-by` operations
- `table-group` → `reduce` with sophisticated accumulators  
- `table-join` → nested `map` and `filter` operations
- `table-pivot` → complex `reduce` with dynamic object construction

### Benefits

1. **SQL-like power** in pure JSON transformation
2. **Functional composition** - operations can be chained and nested
3. **Type safety** - all operations return JSON
4. **Sandboxed** - works within Computo's security model
5. **Debuggable** - can trace through complex data transformations
6. **Memory efficient** - can leverage Computo's memory pooling

### Error Handling

Table operations should provide clear error messages:
- Type mismatches (e.g., trying to sort non-arrays)
- Missing required fields for joins
- Invalid aggregation functions
- Schema validation failures

## Future Extensions

### 1. Schema Validation
```json
["table-validate",
  ["$", "/data"],
  {
    "required": ["id", "name", "email"],
    "types": {"id": "number", "name": "string", "email": "string"}
  }
]
```

### 2. Window Functions
```json
["table-window",
  ["$", "/sales"],
  {
    "partition_by": ["$", "/region"],
    "order_by": ["$", "/date"],
    "functions": {
      "running_total": ["sum", ["$", "/amount"]],
      "rank": ["rank"],
      "previous_value": ["lag", ["$", "/amount"], 1]
    }
  }
]
```

### 3. Statistical Operations
```json
["table-stats",
  ["$", "/measurements"],
  {
    "mean": ["mean", ["$", "/value"]],
    "stddev": ["stddev", ["$", "/value"]],
    "percentiles": ["percentile", ["$", "/value"], [25, 50, 75, 95]]
  }
]
```

## Conclusion

By incorporating tabTools-inspired operations, Computo could become a powerful **"jq meets pandas meets SQL"** tool for JSON transformation. This would enable enterprise-grade data manipulation capabilities while maintaining Computo's functional, sandboxed, and debuggable design principles.

The key insight from tabTools is that **table operations are fundamental data manipulation patterns** that transcend the specific format (CSV, JSON, etc.). By bringing these patterns to JSON processing, Computo could unlock new levels of data transformation capability. 