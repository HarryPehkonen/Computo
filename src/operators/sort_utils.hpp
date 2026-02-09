#pragma once

#include <computo.hpp>
#include <vector>
#include <string>
#include <functional>

namespace computo::operators {

// --- Sort Configuration Structures ---

struct FieldDescriptor {
    std::string pointer;   // JSON Pointer path
    bool ascending = true; // Sort direction
};

struct SortConfig {
    bool is_simple_array;                // Determines sorting mode
    std::string direction;               // For simple arrays
    std::vector<FieldDescriptor> fields; // For object arrays
};

// --- DSU (Decorate-Sort-Undecorate) Structures ---

struct SortItem {
    jsom::JsonDocument original_element;       // Original data for reconstruction
    std::vector<jsom::JsonDocument> sort_keys; // Pre-extracted sort keys

    SortItem(jsom::JsonDocument element, std::vector<jsom::JsonDocument> keys);
};

// Optimized single-field sort item to reduce memory overhead for common case
struct SingleFieldSortItem {
    jsom::JsonDocument original_element;
    jsom::JsonDocument sort_key;

    SingleFieldSortItem(jsom::JsonDocument element, jsom::JsonDocument key);
};

// --- Type Ordering for JSON Comparison ---

enum class JsonTypeOrder : uint8_t {
    Null = 0,
    Number = 1,
    String = 2,
    Boolean = 3,
    Array = 4,
    Object = 5,
    Unknown = 6
};

// --- Sort Utility Functions ---

/**
 * Get the type ordering value for stable cross-type comparison
 */
auto get_type_order(const jsom::JsonDocument& val) -> uint8_t;

/**
 * Compare two JSON values with type-aware ordering
 * Returns: -1 if first < second, 0 if equal, 1 if first > second
 */
auto type_aware_compare(const jsom::JsonDocument& first, const jsom::JsonDocument& second) -> int;

/**
 * Extract a field value from a JSON object using JSON Pointer
 * Returns null if the field is not found
 */
auto extract_sort_field_value(const jsom::JsonDocument& obj, const std::string& pointer) -> jsom::JsonDocument;

/**
 * Parse a field descriptor from various input formats
 * Supports: "/field", ["/field"], ["/field", "asc"], ["/field", "desc"]
 */
auto parse_field_descriptor(const jsom::JsonDocument& field_spec) -> FieldDescriptor;

/**
 * Parse sort operator arguments to determine sorting configuration
 * Handles disambiguation between simple array sorting and object field sorting
 */
auto parse_sort_arguments(const jsom::JsonDocument& args) -> SortConfig;

// --- DSU Comparator Functions ---

/**
 * Create a multi-field comparator for direct object comparison (non-DSU)
 * Used when DSU overhead is not justified
 */
auto create_multi_field_comparator(const std::vector<FieldDescriptor>& fields)
    -> std::function<bool(const jsom::JsonDocument&, const jsom::JsonDocument&)>;

/**
 * Create a comparator for pre-decorated sort items (full DSU)
 */
auto create_dsu_comparator(const std::vector<FieldDescriptor>& fields)
    -> std::function<bool(const SortItem&, const SortItem&)>;

/**
 * Create an optimized comparator for single field DSU sorting
 */
auto create_single_field_dsu_comparator(const FieldDescriptor& field)
    -> std::function<bool(const SingleFieldSortItem&, const SingleFieldSortItem&)>;

// --- Sort Strategy Functions ---

/**
 * Sort simple arrays (primitive values) using direct std::sort
 * Optimal performance for arrays without field-based sorting
 */
auto sort_simple_array(jsom::JsonDocument& array_data, const SortConfig& config) -> void;

/**
 * Sort object arrays using single-field optimized DSU pattern
 * Reduced memory overhead for single field sorting
 */
auto sort_object_array_single_field(jsom::JsonDocument& array_data, const FieldDescriptor& field) -> void;

/**
 * Sort object arrays using full multi-field DSU pattern
 * Pre-extracts all sort keys for O(n) key extraction vs O(n log n)
 */
auto sort_object_array_multi_field(jsom::JsonDocument& array_data, const std::vector<FieldDescriptor>& fields) -> void;

/**
 * Dispatch between single and multi-field object array sorting
 * Chooses the optimal strategy based on field count
 */
auto sort_object_array(jsom::JsonDocument& array_data, const SortConfig& config) -> void;

} // namespace computo::operators
