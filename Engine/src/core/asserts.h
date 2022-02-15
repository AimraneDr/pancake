#pragma once
#include "defines.h"

//disable assertions by commenting the next line
#define PANCAKE_ASSERTIONS_ENABLED

#ifdef PANCAKE_ASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define DebugBreak() __debugbreak()
#else
#define DebugBreak() __builtin_trap()
#endif

PANCAKE_API void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line);

#define PANCAKE_ASSERT(expr)                                         \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            DebugBreak();                                            \
        }                                                            \
    }

#define PANCAKE_ASSERT_MSG(expr, message)                                 \
    {                                                                     \
        if (expr) {                                                       \
        } else {                                                          \
            report_assertion_failure(#expr, message, __FILE__, __LINE__); \
            DebugBreak();                                                 \
        }                                                                 \
    }

#ifdef _DEBUG
#define PANCAKE_ASSERT_DEBUG(expr)                                   \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            DebugBreak();                                            \
        }                                                            \
    }
#else
#define PANCAKE_ASSERT_DEBUG(expr)  // Does nothing at all
#endif

#else
#define PANCAKE_ASSERT(expr)               // Does nothing at all
#define PANCAKE_ASSERT_MSG(expr, message)  // Does nothing at all
#define PANCAKE_ASSERT_DEBUG(expr)         // Does nothing at all
#endif 