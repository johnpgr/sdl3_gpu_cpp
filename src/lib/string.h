#pragma once

#include "def.h"

/// @brief Checks if two strings are equal.
/// @param a The first string.
/// @param b The second string.
/// @return True if the strings are equal, false otherwise.
inline bool string_equals(string a, string b) {
    return strcmp(a, b) == 0;
}

/// @brief Checks if a substring exists within a string.
/// @param haystack The string to search within.
/// @param needle The substring to search for.
/// @return True if the substring is found, false otherwise.
inline bool string_contains(string haystack, string needle) {
    return strstr(haystack, needle) != nullptr;
}

/// @brief Splits a string into substrings based on a delimiter.
/// @param str The input string to split.
/// @param delimiter The character delimiter to split the string on.
/// @param out Pointer to an array of strings to hold the split substrings.
/// @return The number of substrings created.
inline i32 string_split(mut_string str, char delimiter, string* out) {
    if (!str || !out) return 0;

    i32 count = 0;
    mut_string start = str;
    mut_string end = strchr(start, delimiter);

    while (end) {
        *end = '\0';
        *out++ = start;
        count++;
        start = end + 1;
        end = strchr(start, delimiter);
    }

    *out = start;
    count++;

    return count;
}

/// @brief Finds the first occurrence of a substring within a string.
/// @param haystack The string to search within.
/// @param needle The substring to search for.
/// @return The index of the first occurrence, or -1 if not found.
inline i32 string_find(string haystack, string needle) {
    if (!haystack || !needle) return -1;
    
    string result = strstr(haystack, needle);
    if (!result) return -1;
    
    return (i32)(result - haystack);
}

/// @brief Creates a substring from a string into the provided buffer.
/// @param str The source string.
/// @param start The starting index (inclusive).
/// @param length The length of the substring.
/// @param out_buffer The output buffer to write the substring to.
/// @param buffer_size The size of the output buffer.
/// @return True if the substring was successfully created, false otherwise.
inline bool string_substring(string str, i32 start, i32 length, mut_string out_buffer, i32 buffer_size) {
    if (!str || !out_buffer || start < 0 || length < 0 || buffer_size <= 0) return false;
    
    i32 str_len = (i32)strlen(str);
    if (start >= str_len) return false;
    
    // Clamp length to not exceed string bounds
    if (start + length > str_len) {
        length = str_len - start;
    }
    
    // Ensure we don't exceed buffer size (leaving room for null terminator)
    if (length >= buffer_size) {
        length = buffer_size - 1;
    }
    
    errno_t result = strncpy_s(out_buffer, buffer_size, str + start, length);
    return result == 0;
}
