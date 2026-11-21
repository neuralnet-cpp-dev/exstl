// Minimal check harness for exstl standalone tests (no external deps).
#pragma once

#include <cstdio>
#include <string>

inline int g_fail = 0;

#define CHECK(cond, msg)                                                                 \
    do                                                                                   \
    {                                                                                    \
        if (cond)                                                                        \
            std::printf("[ OK ] %s\n", msg);                                             \
        else                                                                             \
        {                                                                                \
            std::printf("[FAIL] %s (%s:%d)\n", msg, __FILE__, __LINE__);                 \
            ++g_fail;                                                                    \
        }                                                                                \
    } while (0)

inline int test_summary(const char* name)
{
    if (g_fail == 0)
    {
        std::printf("=== %s: ALL PASS ===\n", name);
        return 0;
    }
    std::printf("=== %s: %d FAILED ===\n", name, g_fail);
    return 1;
}
