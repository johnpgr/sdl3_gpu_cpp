#pragma once

#include "def.h"
#include <optional>

// TODO: Use these values
enum AllocatorError {
    OutOfMemory,
    InvalidSize,
    InvalidAlignment,
};

struct Allocator {
    void* context;
    void* (*alloc_fn)(void* context, usize size, usize alignment);
    void* (*realloc_fn)(
        void* context,
        void* ptr,
        usize old_size,
        usize new_size,
        usize alignment
    );
    void (*free_fn)(void* context, void* ptr, usize size, usize alignment);

    static Allocator init(
        void* context,
        void* (*alloc_fn)(void* context, usize size, usize alignment),
        void* (*realloc_fn)(
            void* context,
            void* ptr,
            usize old_size,
            usize new_size,
            usize alignment
        ),
        void (*free_fn)(void* context, void* ptr, usize size, usize alignment)
    ) {
        return Allocator{
            .context = context,
            .alloc_fn = alloc_fn,
            .realloc_fn = realloc_fn,
            .free_fn = free_fn,
        };
    }

    // Main allocation methods
    void* alloc(usize size, usize alignment = alignof(void*)) { 
        return alloc_fn(context, size, alignment); 
    }

    void* realloc(void* ptr, usize old_size, usize new_size, usize alignment = alignof(void*)) {
        return realloc_fn(context, ptr, old_size, new_size, alignment);
    }

    void free(void* ptr, usize size, usize alignment = alignof(void*)) {
        free_fn(context, ptr, size, alignment);
    }

    // Convenient methods for typesafe allocations
    template <typename T> 
    T* create() {
        void* ptr = alloc(sizeof(T), alignof(T));
        return (T*)(ptr);
    }

    template <typename T> 
    void destroy(T* ptr) {
        if (ptr) {
            free(ptr, sizeof(T), alignof(T));
        }
    }

    template <typename T> 
    T* alloc_array(usize count) {
        void* ptr = alloc(sizeof(T) * count, alignof(T));
        return (T*)(ptr);
    }

    template <typename T> 
    void free_array(T* ptr, usize count) {
        if (ptr) {
            free(ptr, sizeof(T) * count, alignof(T));
        }
    }
};

struct PageAllocator {
    static Allocator init() {
        return Allocator::init(nullptr, alloc_impl, realloc_impl, free_impl);
    }

  private:
    static void* alloc_impl(void* context, usize size, usize alignment) {
        (void)context;
        (void)alignment;

        return malloc(size);
    }

    static void*
    realloc_impl(void* context, void* ptr, usize old_size, usize new_size, usize alignment) {
        (void)context;
        (void)old_size;
        (void)alignment;
        return ::realloc(ptr, new_size);
    }

    static void free_impl(void* context, void* ptr, usize size, usize alignment) {
        (void)context;
        (void)size;
        (void)alignment;

        ::free(ptr);
    }
};

struct FixedBufferAllocator {
    u8* buffer;
    usize buffer_size;
    usize offset;

    static FixedBufferAllocator init(void* buffer, usize size) {
        return FixedBufferAllocator{
            .buffer = (u8*)(buffer),
            .buffer_size = size,
            .offset = 0,
        };
    }

    // Template version that deduces size from array types
    template<usize N>
    static FixedBufferAllocator init(u8 (&buffer)[N]) {
        return FixedBufferAllocator{
            .buffer = buffer,
            .buffer_size = N,
            .offset = 0,
        };
    }

    // Template version for any array type
    template<typename T, usize N>
    static FixedBufferAllocator init(T (&buffer)[N]) {
        return FixedBufferAllocator{
            .buffer = (u8*)buffer,
            .buffer_size = sizeof(T) * N,
            .offset = 0,
        };
    }

    void deinit() { 
        offset = 0; 
    }

    Allocator allocator() {
        return Allocator::init(this, alloc_impl, realloc_impl, free_impl);
    }

  private:
    static void* alloc_impl(void* context, usize size, usize alignment) {
        FixedBufferAllocator* fba = (FixedBufferAllocator*)(context);

        usize aligned_offset = fba->align_forward(fba->offset, alignment);

        if (aligned_offset + size > fba->buffer_size) {
            // TODO: Better handling of this path here.
            return nullptr; // Out of memory
        }

        void* ptr = fba->buffer + aligned_offset;
        fba->offset = aligned_offset + size;

        return ptr;
    }

    static void*
    realloc_impl(void* context, void* ptr, usize old_size, usize new_size, usize alignment) {
        FixedBufferAllocator* fba = (FixedBufferAllocator*)(context);

        u8* byte_ptr = (u8*)(ptr);

        // Check if this is the last allocation and we can extend in place
        if (byte_ptr + old_size == fba->buffer + fba->offset) {
            if (byte_ptr + new_size <= fba->buffer + fba->buffer_size) {
                fba->offset = (byte_ptr - fba->buffer) + new_size;
                return ptr;
            }
        }

        // Otherwise allocate new memory and copy
        void* new_ptr = alloc_impl(context, new_size, alignment);
        if (new_ptr && ptr) {
            memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
        }

        return new_ptr;
    }

    static void free_impl(void* context, void* ptr, usize size, usize alignment) {
        (void)context;
        (void)ptr;
        (void)size;
        (void)alignment;
        // noop for fixed buffer allocator.
    }

    usize align_forward(usize addr, usize alignment) {
        return (addr + alignment - 1) & ~(alignment - 1);
    }
};

struct ArenaAllocator {
    Allocator child_allocator;

    struct Block {
        Block* next;
        usize size;
        usize offset;
    };

    Block* current_block;
    usize block_size;
    usize max_size;
    usize total_allocated;

    static ArenaAllocator init(Allocator child, usize block_size = 4096, usize max_size = 0) {
        return ArenaAllocator{
            .child_allocator = child,
            .current_block = nullptr,
            .block_size = block_size,
            .max_size = max_size,
            .total_allocated = 0,
        };
    }

    Allocator allocator() {
        return Allocator::init(this, alloc_impl, realloc_impl, free_impl);
    }

    void deinit() {
        Block* block = current_block;
        while (block) {
            Block* next = block->next;
            child_allocator.free(block, block->size + sizeof(Block), alignof(Block));
            block = next;
        }

        current_block = nullptr;
        total_allocated = 0;
    }

  private:
    static void* alloc_impl(void* context, usize size, usize alignment) {
        ArenaAllocator* arena = (ArenaAllocator*)(context);

        if (!arena->ensure_capacity(size, alignment)) {
            // TODO: better handling of this path
            return nullptr; // Out of memory
        }

        usize aligned_offset = arena->align_forward(arena->current_block->offset, alignment);
        void* ptr = (u8*)(arena->current_block + 1) + aligned_offset;
        arena->current_block->offset = aligned_offset + size;

        return ptr;
    }

    static void*
    realloc_impl(void* context, void* ptr, usize old_size, usize new_size, usize alignment) {
        void* new_ptr = alloc_impl(context, new_size, alignment);

        if (new_ptr && ptr) {
            memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
        }

        return new_ptr;
    }

    static void free_impl(void* context, void* ptr, usize size, usize alignment) {
        (void)context;
        (void)ptr;
        (void)size;
        (void)alignment;
        // noop for arena allocator
    }

    bool ensure_capacity(usize size, usize alignment) {
        if (!current_block ||
            align_forward(current_block->offset, alignment) + size > current_block->size) {
            usize new_block_size = size > block_size ? size : block_size;
            usize block_total_size = sizeof(Block) + new_block_size;

            if(max_size > 0 && total_allocated + block_total_size > max_size) {
                return false;
            }

            Block* new_block =
                (Block*)(child_allocator.alloc(block_total_size, alignof(Block)));

            if (!new_block) {
                return false;
            }

            new_block->next = current_block;
            new_block->size = new_block_size;
            new_block->offset = 0;
            current_block = new_block;
            total_allocated += block_total_size;
        }

        return true;
    }

    usize align_forward(usize addr, usize alignment) {
        return (addr + alignment - 1) & ~(alignment - 1);
    }
};

struct GeneralPurposeAllocator {
    struct AllocationInfo {
        usize size;
        bool is_allocated;
    };

    AllocationInfo* allocations;
    usize allocation_count;
    usize max_allocations;
    usize total_allocated;

    static GeneralPurposeAllocator init(usize max_allocations = 1024) {
        AllocationInfo* allocations =
            (AllocationInfo*)(malloc(sizeof(AllocationInfo) * max_allocations));

        for (usize i = 0; i < max_allocations; i++) {
            allocations[i] = AllocationInfo{.size = 0, .is_allocated = false};
        }

        return GeneralPurposeAllocator{
            .allocations = allocations,
            .allocation_count = 0,
            .max_allocations = max_allocations,
            .total_allocated = 0,
        };
    }

    Allocator allocator() {
        return Allocator::init(this, alloc_impl, realloc_impl, free_impl);
    }

    bool deinit() {
        bool has_leaks = allocation_count > 0;
        ::free(allocations);

        allocations = nullptr;
        allocation_count = 0;
        total_allocated = 0;

        return has_leaks;
    }

  private:
    static void* alloc_impl(void* context, usize size, usize alignment) {
        GeneralPurposeAllocator* gpa = (GeneralPurposeAllocator*)(context);
        (void)alignment;

        void* ptr = malloc(size);

        if (ptr) {
            gpa->record_allocation(ptr, size);
        }

        return ptr;
    }

    static void*
    realloc_impl(void* context, void* ptr, usize old_size, usize new_size, usize alignment) {
        GeneralPurposeAllocator* gpa = (GeneralPurposeAllocator*)(context);

        (void)alignment;
        (void)old_size;

        if (ptr) {
            gpa->remove_allocation(ptr);
        }

        void* new_ptr = ::realloc(ptr, new_size);
        if (new_ptr) {
            gpa->record_allocation(new_ptr, new_size);
        }

        return new_ptr;
    }

    static void free_impl(void* context, void* ptr, usize size, usize alignment) {
        GeneralPurposeAllocator* gpa = (GeneralPurposeAllocator*)(context);
        (void)size;
        (void)alignment;

        if (ptr) {
            gpa->remove_allocation(ptr);
            ::free(ptr);
        }
    }

    std::optional<usize> find_allocation(void* ptr) {
        for (usize i = 0; i < max_allocations; i++) {
            if (allocations[i].is_allocated &&
                (void*)((uintptr_t)(&allocations[i]) + sizeof(AllocationInfo)) == ptr) {
                return i;
            }
        }

        return std::nullopt;
    }

    void record_allocation(void* ptr, usize size) {
        (void)ptr;

        for (usize i = 0; i < max_allocations; i++) {
            if (!allocations[i].is_allocated) {
                allocations[i].size = size;
                allocations[i].is_allocated = true;
                allocation_count++;
                total_allocated += size;
                // TODO: Store ptr reference
                // For now its simplified
                break;
            }
        }
    }

    void remove_allocation(void* ptr) {
        auto index = find_allocation(ptr);
        if (index.has_value()) {
            allocations[index.value()].is_allocated = false;
            total_allocated -= allocations[index.value()].size;
            allocation_count--;
        }
    }
};
