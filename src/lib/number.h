#pragma once

#include "def.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

template <typename T>
concept IntegerType =
    std::same_as<T, i8> || std::same_as<T, i16> || std::same_as<T, i32> || std::same_as<T, i64> ||
    std::same_as<T, u8> || std::same_as<T, u16> || std::same_as<T, u32> || std::same_as<T, u64>;

/// @brief Converts a string to an integer.
/// @tparam T The integer type to convert to.
/// @param str The input string to convert.
/// @return The converted integer, or std::nullopt if the conversion failed.
template <IntegerType T> inline std::optional<T> int_from_str(string str) {
    char* endptr;
    int64_t num = strtol(str, &endptr, 10);
    if (endptr == str || *endptr != '\0') {
        return std::nullopt;
    }

    // Check bounds for the target type
    if constexpr (std::is_signed_v<T>) {
        if (num < std::numeric_limits<T>::min() || num > std::numeric_limits<T>::max()) {
            return std::nullopt;
        }
    } else {
        if (num < 0 || static_cast<u64>(num) > std::numeric_limits<T>::max()) {
            return std::nullopt;
        }
    }

    return static_cast<T>(num);
}

template <typename T>
concept FloatType = std::same_as<T, f32> || std::same_as<T, f64>;

/// @brief Converts a string to a floating-point number.
/// @tparam T The floating-point type to convert to.
/// @param str The input string to convert.
/// @return The converted floating-point number, or std::nullopt if the conversion failed.
template <FloatType T> inline std::optional<T> float_from_str(string str) {
    char* endptr;
    double num = strtod(str, &endptr);
    if (endptr == str || *endptr != '\0') {
        return std::nullopt;
    }

    // Check for overflow/underflow
    if (num == HUGE_VAL || num == -HUGE_VAL) {
        return std::nullopt;
    }

    // Check bounds for f32
    if constexpr (std::same_as<T, f32>) {
        if (num > std::numeric_limits<f32>::max() || num < std::numeric_limits<f32>::lowest()) {
            return std::nullopt;
        }
    }

    return static_cast<T>(num);
}
