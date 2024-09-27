#pragma once

#include <cstdint>
#include <cstdlib>
#include <format>
#include <print>
#include <utility>

//===========================================================================
// HANDY TYPEDEFS
//===========================================================================

using s8 = int8_t;
using u8 = uint8_t;

using s16 = int16_t;
using u16 = uint16_t;

using s32 = int32_t;
using u32 = uint32_t;

using s64 = int64_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

//===========================================================================
// MACROS
//===========================================================================

#if defined(_MSC_FULL_VER)
//
// Microsoft Visual Studio
//

#define WIN32_LEAN_AND_MEAN
#include <tchar.h>
#include <windows.h>

#define DEBUG_PRINT(msg) OutputDebugStringA(msg)

constexpr auto msvc_file_name(const char* path) -> const char*
{
    const char* file = path;
    while (*path != 0) {
        if (*path++ == '/') {
            file = path;
        }
    }
    return file;
}

__forceinline void
msvc_panic(const char* file, int line, const char* msg = nullptr)
{
    DEBUG_PRINT(
        std::format("PANIC> {}@{}: {}\n", msvc_file_name(file), line, msg)
            .c_str()
    );

#if defined(DEBUG)
    if (IsDebuggerPresent()) {
        __debugbreak();
    }
#endif // DEBUG

    std::exit(1);
}

#define PANIC_(file, line, msg) msvc_panic(file, line, msg)
#define PANIC() msvc_panic(__FILE__, __LINE__);
#define PANICM(msg) msvc_panic(__FILE__, __LINE__, msg)

[[noreturn]] __forceinline void msvc_unreachable()
{
    __assume(false);
}

#define UNREACHABLE() msvc_unreachable()

template <typename T>
inline T msvc_trueish_or_panic(
    T value,
    bool has_extended_error,
    const char* file,
    int line
) noexcept
{
    if (!value) {
        if (has_extended_error) {
            u32 error = GetLastError();
            auto error_as_hex_string = std::format("{:x}", error);
            PANIC_(file, line, error_as_hex_string.c_str());
        } else {
            PANIC_(file, line, nullptr);
        }
    }
    return value;
}

#if defined(DEBUG)

#define MUST(value) msvc_trueish_or_panic(value, false, __FILE__, __LINE__)
#define MUSTE(value) msvc_trueish_or_panic(value, true, __FILE__, __LINE__)

#else
#define MUST(x) x
#define MUSTE(x) x
#endif // DEBUG

#else

// Not Microsoft Visual Studio

#error Please, check the implementation of these!

#endif // _MSC_FULL_VER

// Macro for generating default constructor
#define DEFAULT_CTOR(ClassName) ClassName() = default

// Macro for generating default destructor
#define DEFAULT_DTOR(ClassName) ~ClassName() = default

// Macro for generating default copy constructor and copy assignment operator
#define DEFAULT_COPY(ClassName)                                                \
    ClassName(const ClassName&) = default;                                     \
    ClassName& operator=(const ClassName&) = default

// Macro for generating default move constructor and move assignment operator
#define DEFAULT_MOVE(ClassName)                                                \
    ClassName(ClassName&&) = default;                                          \
    ClassName& operator=(ClassName&&) = default

// Macro for deleting default constructor
#define DELETE_CTOR(ClassName) ClassName() = delete

// Macro for deleting default destructor
#define DELETE_DTOR(ClassName) ~ClassName() = delete

// Macro for deleting copy constructor and copy assignment operator
#define DELETE_COPY(ClassName)                                                 \
    ClassName(const ClassName&) = delete;                                      \
    ClassName& operator=(const ClassName&) = delete

// Macro for deleting move constructor and move assignment operator
#define DELETE_MOVE(ClassName)                                                 \
    ClassName(ClassName&&) = delete;                                           \
    ClassName& operator=(ClassName&&) = delete
