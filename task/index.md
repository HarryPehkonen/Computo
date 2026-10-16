---
title: Task-Based Operator Index
---

# Task-Based Operator Index

Operators organized by common use cases and functionality:

## Arithmetic

- [`+`](../LANGUAGE_REFERENCE.md#add) - Addition (n-ary)
- [`-`](../LANGUAGE_REFERENCE.md#sub) - Subtraction (n-ary) or unary negation
- [`*`](../LANGUAGE_REFERENCE.md#mul) - Multiplication (n-ary)
- [`/`](../LANGUAGE_REFERENCE.md#div) - Division (n-ary) or reciprocal
- [`%`](../LANGUAGE_REFERENCE.md#mod) - Modulo operation

## Comparison

- [`>`](../LANGUAGE_REFERENCE.md#greater) - Greater than (supports chaining)
- [`<`](../LANGUAGE_REFERENCE.md#less) - Less than (supports chaining)
- [`>=`](../LANGUAGE_REFERENCE.md#greaterequal) - Greater than or equal (supports chaining)
- [`<=`](../LANGUAGE_REFERENCE.md#lessequal) - Less than or equal (supports chaining)
- [`==`](../LANGUAGE_REFERENCE.md#equalequal) - Equality (all arguments must be equal)
- [`!=`](../LANGUAGE_REFERENCE.md#notsubequal) - Inequality (binary only)

## Logical

- [`and`](../LANGUAGE_REFERENCE.md#and) - Logical AND (all must be truthy)
- [`or`](../LANGUAGE_REFERENCE.md#or) - Logical OR (any must be truthy)
- [`not`](../LANGUAGE_REFERENCE.md#not) - Logical NOT (unary)

## Data Access

- [`$input`](../LANGUAGE_REFERENCE.md#dollarsubinput) - Access input data
- [`$inputs`](../LANGUAGE_REFERENCE.md#dollarsubinputs) - Access multiple input files
- [`$`](../LANGUAGE_REFERENCE.md#dollarsub) - Variable access

## Control Flow

- [`if`](../LANGUAGE_REFERENCE.md#if) - Conditional expression
- [`let`](../LANGUAGE_REFERENCE.md#let) - Variable binding

## Lambda Functions

- [`lambda`](../LANGUAGE_REFERENCE.md#lambda) - Lambda function

## Array Operations

- [`map`](../LANGUAGE_REFERENCE.md#map) - Array mapping with lambda
- [`filter`](../LANGUAGE_REFERENCE.md#filter) - Array filtering with lambda
- [`reduce`](../LANGUAGE_REFERENCE.md#reduce) - Array reduction with lambda
- [`count`](../LANGUAGE_REFERENCE.md#count) - Array length
- [`find`](../LANGUAGE_REFERENCE.md#find) - Find first element matching predicate
- [`some`](../LANGUAGE_REFERENCE.md#some) - Test if any element matches predicate
- [`every`](../LANGUAGE_REFERENCE.md#every) - Test if all elements match predicate
- [`append`](../LANGUAGE_REFERENCE.md#append) - Concatenate arrays
- [`sort`](../LANGUAGE_REFERENCE.md#sort) - Array sorting
- [`reverse`](../LANGUAGE_REFERENCE.md#reverse) - Reverse array elements
- [`unique`](../LANGUAGE_REFERENCE.md#unique) - Remove duplicate elements
- [`uniqueSorted`](../LANGUAGE_REFERENCE.md#uniquesorted) - Remove duplicates from sorted array (optimized)
- [`zip`](../LANGUAGE_REFERENCE.md#zip) - Pair corresponding elements from arrays

## Functional Programming

- [`car`](../LANGUAGE_REFERENCE.md#car) - First element of array
- [`cdr`](../LANGUAGE_REFERENCE.md#cdr) - All elements except first
- [`cons`](../LANGUAGE_REFERENCE.md#cons) - Prepend element to array

## Object Operations

- [`obj`](../LANGUAGE_REFERENCE.md#obj) - Object construction (dynamic keys and values)
- [`keys`](../LANGUAGE_REFERENCE.md#keys) - Get object keys
- [`values`](../LANGUAGE_REFERENCE.md#values) - Get object values
- [`objFromPairs`](../LANGUAGE_REFERENCE.md#objfrompairs) - Create object from key-value pairs
- [`pick`](../LANGUAGE_REFERENCE.md#pick) - Select specific object keys
- [`omit`](../LANGUAGE_REFERENCE.md#omit) - Remove specific object keys
- [`merge`](../LANGUAGE_REFERENCE.md#merge) - Merge objects (later objects override earlier)

## String Operations

- [`join`](../LANGUAGE_REFERENCE.md#join) - Join array elements into string
- [`strConcat`](../LANGUAGE_REFERENCE.md#strconcat) - String concatenation

## Utility

- [`approx`](../LANGUAGE_REFERENCE.md#approx) - Approximate equality for numbers
