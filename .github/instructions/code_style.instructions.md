---
applyTo: '**/*.{cpp, c, h, hpp}'
---

# Code style instructions.
## C++ std=c++23

1. Use the following code format style rules from clang-format

---
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
DerivePointerAlignment: false
PointerAlignment: Left
IndentCaseLabels: true
BinPackArguments: false
BinPackParameters: false
AlignAfterOpenBracket: BlockIndent
AllowAllParametersOfDeclarationOnNextLine: false
AllowAllArgumentsOnNextLine: false
AllowShortIfStatementsOnASingleLine: WithoutElse
---

3. Numeric types are aliased to the following:

```cpp
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef size_t usize;
```

4. Only use the `const` keyword when it really makes sense. e.g. fields on a struct that shouldnt be modified, or really CONSTANT things.
5. Only use `auto` when it could improve the readability of the code or reduce redundant typing
```cpp
    auto some_variable = (SomeType*)some_thing; // auto avoids typing the same type two times
    auto some_optional_or_expected_value = random_optional_or_expected_function(); // here it avoids manual typing of an annoying complex return type
```
6. Never depend on any external library to our code.
7. Avoid creating unnecessary abstraction layers. (Virtual functions, inheritance trees)
8. Never use `class`. Only `struct` Never use constructors for initializing and creating a data structure. Always add a ::init() method that is a factory to any struct in the code and a ::deinit() method if the struct needs to be cleaned up.
9. Never use exceptions.
10. Use std::optional or std::expected when a value might be absent
11. Don't use std::string or std::vector. Use raw arrays as much as possible and c style strings.
12. Only use C Style casting e.g. (type)var;
13. Avoid introducing much modern c++ features that might make the code less readable. With an exception of operator overloading and method / function overloading.
14. Allocate memory with a custom allocator struct that should be provided to functions/structs.
    This is what Allocator struct looks like:
```cpp
    struct Allocator {
        void* context;
        void* (*alloc_fn)(void* context, size_t size, size_t alignment);
        void* (*realloc_fn)(void* context, void* ptr, size_t old_size, size_t new_size, size_t alignment);
        void (*free_fn)(void* context, void* ptr, size_t size, size_t alignment);

        static Allocator create(
            void* context,
            void* (*alloc_fn)(void* context, size_t size, size_t alignment),
            void* (*realloc_fn)(void* context, void* ptr, size_t old_size, size_t new_size, size_t alignment),
            void (*free_fn)(void* context, void* ptr, size_t size, size_t alignment)
        );

        // Main allocation methods
        void* alloc(size_t size, size_t alignment = alignof(void*));
        void* realloc(void* ptr, size_t old_size, size_t new_size, size_t alignment = alignof(void*));
        void free(void* ptr, size_t size, size_t alignment = alignof(void*));

        // Convenient methods for typesafe allocations
        template<typename T>
        T* create();

        template<typename T>
        void destroy(T* ptr);

        template<typename T>
        T* alloc_array(size_t count);

        template<typename T>
        void free_array(T* ptr, size_t count);
    };
```

15. Any cleanup code should use the `defer` macro that is defined in the code by `defs.h`.
```cpp
defer { cleanupFunction(); };
```
