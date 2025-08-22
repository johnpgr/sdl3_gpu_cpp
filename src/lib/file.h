#pragma once

#include "allocator.h"
#include "def.h"
#include <cstdio>
#include <optional>

struct File {
    char* data;
    usize size;
    Allocator& allocator;

    /// @brief Deinitializes the File, freeing any allocated memory
    void deinit() {
        if (data) {
            allocator.free_array(data, size + 1); // +1 for null terminator
            data = nullptr;
            size = 0;
        }
    }

    /// @brief Reads the entire contents of a file into memory using the provided allocator
    /// @param filepath Path to the file to read
    /// @param allocator Allocator to use for memory allocation
    /// @return File structure containing the file data, or invalid File on error
    static std::optional<File> read_all(Allocator& allocator, string filepath) {
        if (!filepath) {
            return std::nullopt;
        }

        FILE* file = fopen(filepath, "rb");
        if (!file) {
            return std::nullopt;
        }

        defer { fclose(file); };

        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size <= 0) {
            return std::nullopt;
        }

        // Allocate buffer (add 1 for null terminator)
        char* buffer = allocator.alloc_array<char>((usize)file_size + 1);
        if (!buffer) {
            return std::nullopt;
        }

        // Read file contents
        usize bytes_read = fread(buffer, 1, (usize)file_size, file);
        if (bytes_read != (usize)file_size) {
            allocator.free_array(buffer, (usize)file_size + 1);
            return std::nullopt;
        }

        // Null terminate
        buffer[file_size] = '\0';

        return File{buffer, (usize)file_size, allocator};
    }

    /// @brief Writes data to a file
    /// @param filepath Path to the file to write
    /// @param data Data to write
    /// @param size Size of data in bytes
    /// @return True if successful, false otherwise
    static bool write_all(const char* filepath, const void* data, usize size) {
        if (!filepath || !data || size == 0) {
            return false;
        }

        FILE* file = fopen(filepath, "wb");
        if (!file) {
            return false;
        }

        usize bytes_written = fwrite(data, 1, size, file);

        fclose(file);
        return bytes_written == size;
    }

    /// @brief Checks if a file exists
    /// @param filepath Path to the file to check
    /// @return True if the file exists, false otherwise
    static bool exists(const char* filepath) {
        if (!filepath) {
            return false;
        }

        FILE* file = fopen(filepath, "r");
        if (!file) {
            return false;
        }

        fclose(file);
        return true;
    }

    /// @brief Gets the size of a file in bytes
    /// @param filepath Path to the file
    /// @return Size of the file in bytes, or 0 if file doesn't exist or error
    static usize get_size(const char* filepath) {
        if (!filepath) {
            return 0;
        }

        FILE* file = fopen(filepath, "rb");
        if (!file) {
            return 0;
        }

        fseek(file, 0, SEEK_END);
        long size = ftell(file);

        fclose(file);
        return size > 0 ? (usize)size : 0;
    }

    bool is_valid() const { return data != nullptr && size > 0; }

    const char* c_str() const {
        return data ? data : "";
    }
};
