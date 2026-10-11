#pragma once

// Field-table helper that replaces the Java reflection used on the settings classes
// (util/ReflectionUtil.java). Every settings struct provides
//   - a hand-written `int copy_fields(const X& src, X& dst)` (ReflectionUtil.copyFields), built
//     from the copy_* helpers below, and
//   - a static field table `settings_fields<X>()` listing each public Java field with its
//     `@SerializedName` value and alternates, a getter that formats the value as text and a
//     setter that parses text (ReflectionUtil.setFieldValue / convertValue).
// The field table is reused for JSON load/save (T303), `--router.x.y=v` (T304) and
// `FREEROUTING__ROUTER__X__Y` environment variables (T306).

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace freerouting::settings {

/**
 * Names of the constants of a Java enum used in a settings field, in declaration order.
 * Specialized next to every enum that appears in a settings struct.
 */
template <class E>
struct SettingsEnumNames;

/** One public field of a Java settings class. */
template <class S>
struct SettingsField
{
    /** The Java field name, e.g. "maxPasses". */
    std::string java_name;
    /** The `@SerializedName` value, e.g. "max_passes". */
    std::string json_name;
    /** The `@SerializedName(alternate = ...)` values. */
    std::vector<std::string> alternate_names;
    /**
     * Java `transient`: Gson skips the field in JSON (T303), but copyFields and setFieldValue
     * still handle it.
     */
    bool is_transient = false;
    /**
     * Returns the value as text, or std::nullopt when the field is null (or is a nested
     * object, which has no scalar text form).
     */
    std::function<std::optional<std::string>(const S&)> get;
    /**
     * Sets the field addressed by p_path[p_index] (the path already split at `.`, `:` and `-`).
     * A leaf field parses p_value, a nested object field descends into p_path[p_index + 1].
     * Throws std::invalid_argument like the Java reflection throws its exceptions.
     */
    std::function<void(S&, const std::vector<std::string>& p_path, std::size_t p_index,
                       const std::string& p_value)>
        set;
};

/** The field table of one settings struct; specialized in the struct's .cpp file. */
template <class S>
const std::vector<SettingsField<S>>& settings_fields();

namespace field_table_detail {

inline bool equals_ignore_case(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(a[i]))
            != std::tolower(static_cast<unsigned char>(b[i])))
        {
            return false;
        }
    }
    return true;
}

inline std::string trim(std::string_view p_text)
{
    std::size_t begin = 0;
    std::size_t end = p_text.size();
    // Java String.trim() removes every character <= ' '.
    while (begin < end && static_cast<unsigned char>(p_text[begin]) <= ' ')
    {
        ++begin;
    }
    while (end > begin && static_cast<unsigned char>(p_text[end - 1]) <= ' ')
    {
        --end;
    }
    return std::string(p_text.substr(begin, end - begin));
}

/** Java String.split with a single-character separator: trailing empty strings are removed. */
inline std::vector<std::string> java_split(std::string_view p_text, char p_separator)
{
    std::vector<std::string> result;
    std::size_t start = 0;
    while (true)
    {
        std::size_t pos = p_text.find(p_separator, start);
        if (pos == std::string_view::npos)
        {
            result.emplace_back(p_text.substr(start));
            break;
        }
        result.emplace_back(p_text.substr(start, pos - start));
        start = pos + 1;
    }
    while (!result.empty() && result.back().empty())
    {
        result.pop_back();
    }
    return result;
}

} // namespace field_table_detail

/** Java ReflectionUtil.snakeToLowerCamel. */
inline std::string snake_to_lower_camel(std::string_view p_name)
{
    if (p_name.find('_') == std::string_view::npos)
    {
        return std::string(p_name);
    }
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true)
    {
        std::size_t pos = p_name.find('_', start);
        if (pos == std::string_view::npos)
        {
            parts.emplace_back(p_name.substr(start));
            break;
        }
        parts.emplace_back(p_name.substr(start, pos - start));
        start = pos + 1;
    }
    // Java String.split drops trailing empty parts.
    while (!parts.empty() && parts.back().empty())
    {
        parts.pop_back();
    }
    std::string result;
    if (parts.empty())
    {
        return result;
    }
    for (char c : parts[0])
    {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    for (std::size_t i = 1; i < parts.size(); ++i)
    {
        if (parts[i].empty())
        {
            continue;
        }
        result += static_cast<char>(std::toupper(static_cast<unsigned char>(parts[i][0])));
        for (std::size_t j = 1; j < parts[i].size(); ++j)
        {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(parts[i][j])));
        }
    }
    return result;
}

/**
 * Java ReflectionUtil.getFieldByNameOrSerializedName: matches p_name (case-insensitive) against
 * the serialized name, its alternates and the Java field name, each also in lower-camel form.
 */
template <class S>
bool field_matches_name(const SettingsField<S>& p_field, std::string_view p_name)
{
    using field_table_detail::equals_ignore_case;
    const std::string camel_name = snake_to_lower_camel(p_name);
    auto matches_annotation = [&](const std::string& p_value) {
        return equals_ignore_case(p_value, p_name) || equals_ignore_case(p_value, camel_name)
            || equals_ignore_case(snake_to_lower_camel(p_value), p_name)
            || equals_ignore_case(snake_to_lower_camel(p_value), camel_name);
    };
    if (matches_annotation(p_field.json_name))
    {
        return true;
    }
    for (const auto& alternate : p_field.alternate_names)
    {
        if (matches_annotation(alternate))
        {
            return true;
        }
    }
    return equals_ignore_case(p_field.java_name, p_name)
        || equals_ignore_case(p_field.java_name, camel_name)
        || equals_ignore_case(snake_to_lower_camel(p_field.java_name), camel_name);
}

/** Looks up a field like Java getFieldByNameOrSerializedName; nullptr when there is none. */
template <class S>
const SettingsField<S>* find_settings_field(std::string_view p_name)
{
    for (const auto& field : settings_fields<S>())
    {
        if (field_matches_name(field, p_name))
        {
            return &field;
        }
    }
    return nullptr;
}

/** Splits a property path at `.`, `:` and `-` like Java `propertyName.split("[.:\\-]")`. */
inline std::vector<std::string> split_property_path(std::string_view p_property_name)
{
    std::vector<std::string> result;
    std::string current;
    for (char c : p_property_name)
    {
        if (c == '.' || c == ':' || c == '-')
        {
            result.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }
    result.push_back(current);
    while (!result.empty() && result.back().empty())
    {
        result.pop_back();
    }
    return result;
}

/** Recursive step of set_field_value (Java ReflectionUtil.setPropertyRecursive). */
template <class S>
void set_field_value_at(S& p_object, const std::vector<std::string>& p_path, std::size_t p_index,
                        const std::string& p_value)
{
    if (p_index >= p_path.size())
    {
        throw std::invalid_argument("Empty property path");
    }
    const SettingsField<S>* field = find_settings_field<S>(p_path[p_index]);
    if (field == nullptr)
    {
        throw std::invalid_argument("No field found with name or SerializedName: " + p_path[p_index]);
    }
    field->set(p_object, p_path, p_index, p_value);
}

/**
 * Java ReflectionUtil.setFieldValue: sets a nested field from text using a property path such as
 * "optimizer.max_passes". Throws std::invalid_argument when the path or the value is invalid.
 */
template <class S>
void set_field_value(S& p_object, std::string_view p_property_name, const std::string& p_value)
{
    set_field_value_at(p_object, split_property_path(p_property_name), 0, p_value);
}

// ---------------------------------------------------------------------------------------------
// Text conversion (Java ReflectionUtil.convertValue and Java's toString of the boxed values)
// ---------------------------------------------------------------------------------------------

/** Integer.parseInt / Long.parseLong: optional sign followed by decimal digits only. */
template <class T>
T parse_java_integer(const std::string& p_text)
{
    std::string_view view = p_text;
    if (!view.empty() && view.front() == '+')
    {
        view.remove_prefix(1);
        if (!view.empty() && view.front() == '-')
        {
            throw std::invalid_argument("For input string: \"" + p_text + "\"");
        }
    }
    T result{};
    auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), result);
    if (view.empty() || ec != std::errc() || ptr != view.data() + view.size())
    {
        throw std::invalid_argument("For input string: \"" + p_text + "\"");
    }
    return result;
}

/** Double.parseDouble / Float.parseFloat: surrounding whitespace is ignored. */
template <class T>
T parse_java_floating(const std::string& p_text)
{
    std::string text = field_table_detail::trim(p_text);
    // Java accepts a trailing type suffix (d, D, f, F).
    if (!text.empty()
        && (text.back() == 'd' || text.back() == 'D' || text.back() == 'f' || text.back() == 'F'))
    {
        text.pop_back();
    }
    if (text == "Infinity" || text == "+Infinity")
    {
        return std::numeric_limits<T>::infinity();
    }
    if (text == "-Infinity")
    {
        return -std::numeric_limits<T>::infinity();
    }
    if (text == "NaN")
    {
        return std::numeric_limits<T>::quiet_NaN();
    }
    if (text.empty() || !(std::isdigit(static_cast<unsigned char>(text.back())) || text.back() == '.'))
    {
        throw std::invalid_argument("For input string: \"" + p_text + "\"");
    }
    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (end != text.c_str() + text.size())
    {
        throw std::invalid_argument("For input string: \"" + p_text + "\"");
    }
    return static_cast<T>(value);
}

/** Java Boolean conversion of convertValue: "0"/"1" first, then Boolean.parseBoolean. */
inline bool parse_java_boolean(const std::string& p_text)
{
    if (p_text == "0")
    {
        return false;
    }
    if (p_text == "1")
    {
        return true;
    }
    return field_table_detail::equals_ignore_case(p_text, "true");
}

/**
 * Shortest round-trip text of a floating point value, with Java's ".0" for integral values.
 * Java parity: Double.toString switches to scientific notation outside [1e-3, 1e7); that form is
 * not reproduced here (only used for logging and JSON output).
 */
template <class T>
std::string format_java_floating(T p_value)
{
    if (std::isnan(p_value))
    {
        return "NaN";
    }
    if (std::isinf(p_value))
    {
        return p_value > 0 ? "Infinity" : "-Infinity";
    }
    char buffer[64];
    auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), p_value);
    std::string result(buffer, ptr);
    if (result.find_first_of(".e") == std::string::npos)
    {
        result += ".0";
    }
    return result;
}

template <class T>
struct SettingsValueConverter;

template <>
struct SettingsValueConverter<bool>
{
    static bool parse(const std::string& p_text) { return parse_java_boolean(p_text); }
    static std::string format(bool p_value) { return p_value ? "true" : "false"; }
};

template <>
struct SettingsValueConverter<int>
{
    static int parse(const std::string& p_text) { return parse_java_integer<int>(p_text); }
    static std::string format(int p_value) { return std::to_string(p_value); }
};

template <>
struct SettingsValueConverter<long long>
{
    static long long parse(const std::string& p_text) { return parse_java_integer<long long>(p_text); }
    static std::string format(long long p_value) { return std::to_string(p_value); }
};

template <>
struct SettingsValueConverter<float>
{
    static float parse(const std::string& p_text) { return parse_java_floating<float>(p_text); }
    static std::string format(float p_value) { return format_java_floating(p_value); }
};

template <>
struct SettingsValueConverter<double>
{
    static double parse(const std::string& p_text) { return parse_java_floating<double>(p_text); }
    static std::string format(double p_value) { return format_java_floating(p_value); }
};

template <>
struct SettingsValueConverter<std::string>
{
    static std::string parse(const std::string& p_text) { return p_text; }
    static std::string format(const std::string& p_value) { return p_value; }
};

/** String[] / double[]: comma separated, trimmed, an empty text is an empty array. */
template <class T>
struct SettingsValueConverter<std::vector<T>>
{
    static std::vector<T> parse(const std::string& p_text)
    {
        std::string raw = field_table_detail::trim(p_text);
        std::vector<T> result;
        if (raw.empty())
        {
            return result;
        }
        for (const auto& token : field_table_detail::java_split(raw, ','))
        {
            result.push_back(SettingsValueConverter<T>::parse(field_table_detail::trim(token)));
        }
        return result;
    }

    static std::string format(const std::vector<T>& p_value)
    {
        std::string result;
        for (std::size_t i = 0; i < p_value.size(); ++i)
        {
            if (i > 0)
            {
                result += ",";
            }
            result += SettingsValueConverter<T>::format(p_value[i]);
        }
        return result;
    }
};

/** Enums: case-insensitive match of the trimmed text against the Java constant names. */
template <class E>
    requires std::is_enum_v<E>
struct SettingsValueConverter<E>
{
    static E parse(const std::string& p_text)
    {
        const std::string text = field_table_detail::trim(p_text);
        const auto& names = SettingsEnumNames<E>::names;
        for (std::size_t i = 0; i < names.size(); ++i)
        {
            if (field_table_detail::equals_ignore_case(names[i].second, text))
            {
                return names[i].first;
            }
        }
        throw std::invalid_argument("No enum constant for value: " + p_text);
    }

    static std::string format(E p_value)
    {
        for (const auto& entry : SettingsEnumNames<E>::names)
        {
            if (entry.first == p_value)
            {
                return std::string(entry.second);
            }
        }
        throw std::invalid_argument("Enum value without a Java name");
    }
};

// ---------------------------------------------------------------------------------------------
// Field table entry factories
// ---------------------------------------------------------------------------------------------

/** A leaf field (boxed scalar, String, enum or primitive/String array) stored as std::optional. */
template <class S, class T>
SettingsField<S> settings_leaf_field(std::string p_java_name, std::string p_json_name,
                                     std::optional<T> S::*p_member,
                                     std::vector<std::string> p_alternate_names = {})
{
    SettingsField<S> field;
    field.java_name = std::move(p_java_name);
    field.json_name = std::move(p_json_name);
    field.alternate_names = std::move(p_alternate_names);
    field.get = [p_member](const S& p_object) -> std::optional<std::string> {
        const auto& value = p_object.*p_member;
        if (!value)
        {
            return std::nullopt;
        }
        return SettingsValueConverter<T>::format(*value);
    };
    field.set = [p_member, name = field.json_name](S& p_object, const std::vector<std::string>& p_path,
                                                   std::size_t p_index, const std::string& p_value) {
        if (p_index + 1 != p_path.size())
        {
            throw std::invalid_argument("No field found with name or SerializedName: " + p_path[p_index + 1]
                                        + " (" + name + " is not an object)");
        }
        p_object.*p_member = SettingsValueConverter<T>::parse(p_value);
    };
    return field;
}

/** A nested settings object field (Java: a non-null object field that setFieldValue descends into). */
template <class S, class N>
SettingsField<S> settings_object_field(std::string p_java_name, std::string p_json_name, N S::*p_member)
{
    SettingsField<S> field;
    field.java_name = std::move(p_java_name);
    field.json_name = std::move(p_json_name);
    field.get = [](const S&) -> std::optional<std::string> { return std::nullopt; };
    field.set = [p_member, name = field.json_name](S& p_object, const std::vector<std::string>& p_path,
                                                   std::size_t p_index, const std::string& p_value) {
        if (p_index + 1 >= p_path.size())
        {
            // Java parity: convertValue returns the String unchanged and Field.set throws.
            throw std::invalid_argument("Cannot set object field " + name + " from text");
        }
        set_field_value_at(p_object.*p_member, p_path, p_index + 1, p_value);
    };
    return field;
}

/**
 * An array-of-objects field (Java LayerSettings[]). Navigating through it applies one
 * comma-separated value token per element, like ReflectionUtil.setPropertyRecursive.
 */
template <class S, class N>
SettingsField<S> settings_object_array_field(std::string p_java_name, std::string p_json_name,
                                             std::optional<std::vector<N>> S::*p_member)
{
    SettingsField<S> field;
    field.java_name = std::move(p_java_name);
    field.json_name = std::move(p_json_name);
    field.get = [](const S&) -> std::optional<std::string> { return std::nullopt; };
    field.set = [p_member, name = field.json_name](S& p_object, const std::vector<std::string>& p_path,
                                                   std::size_t p_index, const std::string& p_value) {
        if (p_index + 1 >= p_path.size())
        {
            throw std::invalid_argument("Cannot set object array field " + name + " from text");
        }
        const auto tokens = field_table_detail::java_split(p_value, ',');
        auto& array = p_object.*p_member;
        if (!array)
        {
            array = std::vector<N>(tokens.size());
        }
        const std::size_t limit = std::min(array->size(), tokens.size());
        for (std::size_t i = 0; i < limit; ++i)
        {
            set_field_value_at((*array)[i], p_path, p_index + 1, field_table_detail::trim(tokens[i]));
        }
    };
    return field;
}

/** Marks a field table entry as Java `transient`. */
template <class S>
SettingsField<S> settings_transient(SettingsField<S> p_field)
{
    p_field.is_transient = true;
    return p_field;
}

// ---------------------------------------------------------------------------------------------
// copy_fields helpers (Java ReflectionUtil.copyFields, one branch per field kind)
// ---------------------------------------------------------------------------------------------

/**
 * Boxed scalar or String field: copied when the source value is non-null.
 * Java parity: Java counts the copy when `targetValue != sourceValue` compares unequal
 * *references*; equal boxed values that are not the same object (e.g. two Double objects) are
 * therefore counted by Java but not here. The copied value is the same either way.
 */
template <class T>
int copy_boxed_field(const std::optional<T>& p_source, std::optional<T>& p_target)
{
    if (!p_source)
    {
        return 0;
    }
    if (p_target && *p_target == *p_source)
    {
        return 0;
    }
    p_target = p_source;
    return 1;
}

/** Enum field: a non-null source value is always copied and always counted. */
template <class E>
int copy_enum_field(const std::optional<E>& p_source, std::optional<E>& p_target)
{
    if (!p_source)
    {
        return 0;
    }
    p_target = p_source;
    return 1;
}

/** Array of primitives or Strings: copied when the target is null or the source is non-empty. */
template <class T>
int copy_array_field(const std::optional<std::vector<T>>& p_source, std::optional<std::vector<T>>& p_target)
{
    if (!p_source)
    {
        return 0;
    }
    if (!p_target || !p_source->empty())
    {
        p_target = p_source;
        return 1;
    }
    return 0;
}

/**
 * Array of objects (LayerSettings[]): merged element-wise into a long-enough target, otherwise a
 * new target array of the source length is built from copies into fresh elements. Counts the
 * source length in both cases. `p_copy` is the element type's copy_fields.
 */
template <class N, class CopyFn>
int copy_object_array_field(const std::optional<std::vector<N>>& p_source,
                            std::optional<std::vector<N>>& p_target, CopyFn p_copy)
{
    if (!p_source)
    {
        return 0;
    }
    const std::size_t target_length = p_target ? p_target->size() : 0;
    if (target_length >= p_source->size())
    {
        for (std::size_t i = 0; i < p_source->size(); ++i)
        {
            p_copy((*p_source)[i], (*p_target)[i]);
        }
    }
    else
    {
        std::vector<N> new_target(p_source->size());
        for (std::size_t i = 0; i < p_source->size(); ++i)
        {
            p_copy((*p_source)[i], new_target[i]);
        }
        p_target = std::move(new_target);
    }
    return static_cast<int>(p_source->size());
}

} // namespace freerouting::settings
