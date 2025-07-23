#include "sort_utils.hpp"
#include "operators/shared.hpp"
#include <algorithm>

namespace computo::operators {

// --- Sort Item Constructors ---

SortItem::SortItem(nlohmann::json element, std::vector<nlohmann::json> keys)
    : original_element(std::move(element)), sort_keys(std::move(keys)) {}

SingleFieldSortItem::SingleFieldSortItem(nlohmann::json element, nlohmann::json key)
    : original_element(std::move(element)), sort_key(std::move(key)) {}

// --- Type Ordering Functions ---

auto get_type_order(const nlohmann::json& val) -> uint8_t {
    if (val.is_null()) {
        return static_cast<uint8_t>(JsonTypeOrder::Null);
    }
    if (val.is_number()) {
        return static_cast<uint8_t>(JsonTypeOrder::Number);
    }
    if (val.is_string()) {
        return static_cast<uint8_t>(JsonTypeOrder::String);
    }
    if (val.is_boolean()) {
        return static_cast<uint8_t>(JsonTypeOrder::Boolean);
    }
    if (val.is_array()) {
        return static_cast<uint8_t>(JsonTypeOrder::Array);
    }
    if (val.is_object()) {
        return static_cast<uint8_t>(JsonTypeOrder::Object);
    }
    return static_cast<uint8_t>(JsonTypeOrder::Unknown);
}

// NOLINTBEGIN(readability-function-size)
auto type_aware_compare(const nlohmann::json& first, const nlohmann::json& second) -> int {
    uint8_t type_first = get_type_order(first);
    uint8_t type_second = get_type_order(second);

    if (type_first != type_second) {
        return static_cast<int>(type_first) - static_cast<int>(type_second); // Different types
    }

    // Same type, compare values
    if (first.is_number()) {
        double val_first = first.get<double>();
        double val_second = second.get<double>();
        if (val_first < val_second) {
            return -1;
        }
        if (val_first > val_second) {
            return 1;
        }
        return 0;
    }
    if (first.is_string()) {
        std::string str_first = first.get<std::string>();
        std::string str_second = second.get<std::string>();
        return str_first.compare(str_second);
    }
    if (first.is_boolean()) {
        bool bool_first = first.get<bool>();
        bool bool_second = second.get<bool>();
        if (static_cast<int>(bool_first) < static_cast<int>(bool_second)) {
            return -1;
        }
        if (static_cast<int>(bool_first) > static_cast<int>(bool_second)) {
            return 1;
        }
        return 0;
    }
    // For arrays and objects, use JSON's built-in comparison
    if (first < second) {
        return -1;
    }
    if (first > second) {
        return 1;
    }
    return 0;
}
// NOLINTEND(readability-function-size)

// --- Field Extraction and Parsing ---

auto extract_sort_field_value(const nlohmann::json& obj, const std::string& pointer)
    -> nlohmann::json {
    if (pointer.empty()) {
        return obj; // Empty pointer means the value itself
    }

    try {
        return obj.at(nlohmann::json::json_pointer(pointer));
    } catch (const std::exception&) {
        return nlohmann::json{}; // Field not found, return null
    }
}

// NOLINTBEGIN(readability-function-size)
auto parse_field_descriptor(const nlohmann::json& field_spec) -> FieldDescriptor {
    FieldDescriptor desc;

    if (field_spec.is_string()) {
        // Simple string field: "/field" -> ascending
        desc.pointer = field_spec.get<std::string>();
        desc.ascending = true;
    } else if (field_spec.is_array()) {
        // Array format: ["/field", "desc"] or ["/field", "asc"]
        if (field_spec.empty()) {
            throw InvalidArgumentException(
                R"(Invalid field descriptor: empty array. Expected ["/field"] or ["/field", "asc|desc"]")",
                "");
        }
        if (!field_spec[0].is_string()) {
            throw InvalidArgumentException("Invalid field descriptor: first "
                                           "element must be a string, got: "
                                               + field_spec[0].dump(),
                                           "");
        }

        desc.pointer = field_spec[0].get<std::string>();
        desc.ascending = true; // default

        if (field_spec.size() > 1) {
            if (!field_spec[1].is_string()) {
                throw InvalidArgumentException(
                    "Invalid field descriptor: direction must be a string, "
                    "got: "
                        + field_spec[1].dump(),
                    "");
            }
            std::string direction = field_spec[1].get<std::string>();
            if (direction != "asc" && direction != "desc") {
                throw InvalidArgumentException(
                    "Invalid sort direction: '" + direction + "'. Must be 'asc' or 'desc'", "");
            }
            desc.ascending = (direction != "desc");
        }
    } else {
        throw InvalidArgumentException(
            "Field descriptor must be string or array, got: " + field_spec.dump(), "");
    }

    return desc;
}
// NOLINTEND(readability-function-size)

// NOLINTBEGIN(readability-function-size)
auto parse_sort_arguments(const nlohmann::json& args) -> SortConfig {
    SortConfig config;

    if (args.size() == 1) {
        // Simple array sort, ascending
        config.is_simple_array = true;
        config.direction = "asc";
    } else if (args.size() == 2) {
        // Disambiguation logic
        if (args[1].is_string()) {
            std::string second_arg = args[1].get<std::string>();
            if (second_arg == "desc" || second_arg == "asc") {
                // Simple array with direction
                config.is_simple_array = true;
                config.direction = second_arg;
            } else if (second_arg.empty() || second_arg[0] == '/') {
                // Object array with JSON Pointer field
                config.is_simple_array = false;
                config.fields.push_back(parse_field_descriptor(args[1]));
            } else {
                throw InvalidArgumentException("Invalid sort argument: " + second_arg, "");
            }
        } else {
            // Object array with array-format field descriptor
            config.is_simple_array = false;
            config.fields.push_back(parse_field_descriptor(args[1]));
        }
    } else {
        // Multiple fields for object sorting
        config.is_simple_array = false;
        for (size_t i = 1; i < args.size(); ++i) {
            config.fields.push_back(parse_field_descriptor(args[i]));
        }
    }

    return config;
}
// NOLINTEND(readability-function-size)

// --- DSU Comparator Functions ---

auto create_multi_field_comparator(const std::vector<FieldDescriptor>& fields)
    -> std::function<bool(const nlohmann::json&, const nlohmann::json&)> {

    return [fields](const nlohmann::json& left, const nlohmann::json& right) -> bool {
        for (const auto& field : fields) {
            auto val_left = extract_sort_field_value(left, field.pointer);
            auto val_right = extract_sort_field_value(right, field.pointer);

            int cmp = type_aware_compare(val_left, val_right);
            if (cmp != 0) {
                return field.ascending ? (cmp < 0) : (cmp > 0);
            }
            // Fields are equal, continue to next field
        }
        return false; // All fields equal
    };
}

auto create_dsu_comparator(const std::vector<FieldDescriptor>& fields)
    -> std::function<bool(const SortItem&, const SortItem&)> {

    return [fields](const SortItem& left, const SortItem& right) -> bool {
        for (size_t i = 0; i < fields.size(); ++i) {
            const auto& field = fields[i];

            // Keys are pre-extracted in SortItem.sort_keys
            int cmp = type_aware_compare(left.sort_keys[i], right.sort_keys[i]);
            if (cmp != 0) {
                return field.ascending ? (cmp < 0) : (cmp > 0);
            }
            // Keys are equal, continue to next field
        }
        return false; // All keys equal
    };
}

auto create_single_field_dsu_comparator(const FieldDescriptor& field)
    -> std::function<bool(const SingleFieldSortItem&, const SingleFieldSortItem&)> {

    return [field](const SingleFieldSortItem& left, const SingleFieldSortItem& right) -> bool {
        int cmp = type_aware_compare(left.sort_key, right.sort_key);
        return field.ascending ? (cmp < 0) : (cmp > 0);
    };
}

// --- Sort Strategy Functions ---

auto sort_simple_array(nlohmann::json& array_data, const SortConfig& config) -> void {
    // Simple array sorting - use direct std::sort for optimal performance
    bool ascending = (config.direction == "asc");
    std::sort(array_data.begin(), array_data.end(),
              [ascending](const nlohmann::json& left, const nlohmann::json& right) {
                  int cmp = type_aware_compare(left, right);
                  return ascending ? (cmp < 0) : (cmp > 0);
              });
}

auto sort_object_array_single_field(nlohmann::json& array_data, const FieldDescriptor& field) -> void {
    std::vector<SingleFieldSortItem> decorated_items;
    decorated_items.reserve(array_data.size());

    // Decorate: Extract single sort key
    for (const auto& element : array_data) {
        auto key = extract_sort_field_value(element, field.pointer);
        decorated_items.emplace_back(element, std::move(key));
    }

    // Sort: Use pre-extracted key
    auto single_field_comparator = create_single_field_dsu_comparator(field);
    std::sort(decorated_items.begin(), decorated_items.end(), single_field_comparator);

    // Undecorate: Extract sorted original elements
    array_data = nlohmann::json::array();
    for (const auto& item : decorated_items) {
        array_data.push_back(item.original_element);
    }
}

auto sort_object_array_multi_field(nlohmann::json& array_data, const std::vector<FieldDescriptor>& fields) -> void {
    std::vector<SortItem> decorated_items;
    decorated_items.reserve(array_data.size());

    // Decorate: Extract all sort keys upfront (O(n) instead of O(n log n))
    for (const auto& element : array_data) {
        std::vector<nlohmann::json> keys;
        keys.reserve(fields.size());

        for (const auto& field : fields) {
            keys.push_back(extract_sort_field_value(element, field.pointer));
        }

        decorated_items.emplace_back(element, std::move(keys));
    }

    // Sort: Use pre-extracted keys for O(n log n) comparisons
    auto dsu_comparator = create_dsu_comparator(fields);
    std::sort(decorated_items.begin(), decorated_items.end(), dsu_comparator);

    // Undecorate: Extract sorted original elements
    array_data = nlohmann::json::array();
    for (const auto& item : decorated_items) {
        array_data.push_back(item.original_element);
    }
}

auto sort_object_array(nlohmann::json& array_data, const SortConfig& config) -> void {
    if (config.fields.size() == 1) {
        sort_object_array_single_field(array_data, config.fields[0]);
    } else {
        sort_object_array_multi_field(array_data, config.fields);
    }
}

} // namespace computo::operators
