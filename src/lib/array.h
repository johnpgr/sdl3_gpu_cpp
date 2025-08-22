#pragma once

#include "allocator.h"
#include "def.h"
#include <cstring>
#include <optional>

template <typename T> struct ArrayList {
    T* items;
    usize len;
    usize capacity;
    usize max_items;
    Allocator allocator;

    static ArrayList<T> init(Allocator allocator) {
        return ArrayList<T>{
            .items = nullptr,
            .len = 0,
            .capacity = 0,
            .max_items = USIZE_MAX,
            .allocator = allocator,
        };
    }

    static ArrayList<T> init(Allocator allocator, usize max_items) {
        return ArrayList<T>{
            .items = nullptr,
            .len = 0,
            .capacity = 0,
            .max_items = max_items,
            .allocator = allocator,
        };
    }

    static ArrayList<T> init_capacity(Allocator allocator, usize capacity) {
        T* items = nullptr;

        if (capacity > 0) {
            items = allocator.alloc_array<T>(capacity);
            if (!items) {
                return ArrayList<T>{
                    .items = nullptr,
                    .len = 0,
                    .capacity = 0,
                    .max_items = USIZE_MAX,
                    .allocator = allocator,
                };
            }
        }

        return ArrayList<T>{
            .items = items,
            .len = 0,
            .capacity = capacity,
            .max_items = USIZE_MAX,
            .allocator = allocator,
        };
    }

    static ArrayList<T> init_capacity(Allocator allocator, usize capacity, usize max_items) {
        // Ensure initial capacity doesn't exceed max_items
        if (capacity > max_items) {
            capacity = max_items;
        }

        T* items = nullptr;
        if (capacity > 0) {
            items = allocator.alloc_array<T>(capacity);
            if (!items) {
                return ArrayList<T>{
                    .items = nullptr,
                    .len = 0,
                    .capacity = 0,
                    .max_items = max_items,
                    .allocator = allocator,
                };
            }
        }

        return ArrayList<T>{
            .items = items,
            .len = 0,
            .capacity = capacity,
            .max_items = max_items,
            .allocator = allocator,
        };
    }

    void deinit() {
        if (items) {
            allocator.free_array(items, capacity);
        }
        items = nullptr;
        len = 0;
        capacity = 0;
    }

    bool append(T item) {
        if (len >= max_items) return false;
        if (!ensure_capacity(len + 1)) return false;

        items[len] = item;
        len++;
        return true;
    }

    std::optional<T> pop() {
        if (len == 0) return std::nullopt;
        len--;
        return items[len];
    }

    bool insert(usize index, T item) {
        if (index > len) return false;
        if (len >= max_items) return false;
        if (!ensure_capacity(len + 1)) return false;

        // Shift elements to the right
        for (usize i = len; i > index; i--) {
            items[i] = items[i - 1];
        }

        items[index] = item;
        len++;
        return true;
    }

    std::optional<T> ordered_remove(usize index) {
        if (index >= len) return std::nullopt;

        T item = items[index];

        // Shift elements to the left
        for (usize i = index; i < len - 1; i++) {
            items[i] = items[i + 1];
        }

        len--;
        return item;
    }

    std::optional<T> swap_remove(usize index) {
        if (index >= len) return std::nullopt;

        T item = items[index];
        items[index] = items[len - 1];
        len--;
        return item;
    }

    void clear() { len = 0; }

    bool resize(usize new_len) {
        if (new_len > max_items) return false;

        if (new_len > capacity)
            if (!ensure_capacity(new_len)) return false;

        len = new_len;
        return true;
    }

    bool reserve(usize additional_capacity) {
        usize new_capacity = len + additional_capacity;
        if (new_capacity > max_items) new_capacity = max_items;

        return ensure_capacity(new_capacity);
    }

    void shrink_to_fit() {
        if (len == capacity) return;

        if (len == 0) {
            if (items) {
                allocator.free_array(items, capacity);
                items = nullptr;
            }
            capacity = 0;
            return;
        }

        T* new_items = allocator.alloc_array<T>(len);
        if (!new_items) return; // Keep old allocation if realloc fails
        memcpy(new_items, items, sizeof(T) * len);
        allocator.free_array(items, capacity);
        items = new_items;
        capacity = len;
    }

    std::optional<T> operator[](usize index) { 
        if (index >= len) return std::nullopt;
        return items[index]; 
    }

    std::optional<T> operator[](usize index) const { 
        if (index >= len) return std::nullopt;
        return items[index]; 
    }

    T* slice(usize start, usize end = USIZE_MAX) {
        if (end == USIZE_MAX) end = len;

        if (start > len || end > len || start > end) return nullptr;

        return items + start;
    }

    T* begin() { return items; }
    T* end() { return items + len; }
    T* begin() const { return items; }
    T* end() const { return items + len; }

  private:
    bool ensure_capacity(usize min_capacity) {
        if (min_capacity <= capacity) return true;

        if (min_capacity > max_items) return false;

        usize new_capacity = capacity;

        if (new_capacity == 0) new_capacity = 8;

        while (new_capacity < min_capacity)
            new_capacity *= 2;

        T* new_items = allocator.alloc_array<T>(new_capacity);
        if (!new_items) return false;

        if (items && len > 0) memcpy(new_items, items, sizeof(T) * len);

        if (items) allocator.free_array(items, capacity);

        items = new_items;
        capacity = new_capacity;
        return true;
    }
};
