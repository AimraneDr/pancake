#include "tests_manager.h"

#include <containers\list.h>
#include <core/logger.h>
#include <core/pancake_string.h>
#include <core/clock.h>

typedef struct test_entry {
    PFN_test func;
    char* desc;
} test_entry;

static test_entry* tests;

void test_manager_init() {
    tests = list_create(test_entry);
}

void test_manager_register_test(u8 (*PFN_test)(), char* desc) {
    test_entry e;
    e.func = PFN_test;
    e.desc = desc;
    list_push(tests, e);
}

void test_manager_run_tests() {
    u32 passed = 0;
    u32 failed = 0;
    u32 skipped = 0;

    u32 count = list_length(tests);

    Clock total_time;
    clock_start(&total_time);

    for (u32 i = 0; i < count; ++i) {
        Clock test_time;
        clock_start(&test_time);
        u8 result = tests[i].func();
        clock_update(&test_time);

        if (result == true) {
            ++passed;
        } else if (result == BYPASS) {
            PANCAKE_WARN("[SKIPPED]: %s", tests[i].desc);
            ++skipped;
        } else {
            PANCAKE_ERROR("[FAILED]: %s", tests[i].desc);
            ++failed;
        }
        char status[20];
        string_format(status, failed ? "*** %d FAILED ***" : "SUCCESS", failed);
        clock_update(&total_time);
        PANCAKE_INFO("Executed %d of %d (skipped %d) %s (%.6f sec / %.6f sec total", i + 1, count, skipped, status, test_time.elapsed, total_time.elapsed);
    }

    clock_stop(&total_time);

    PANCAKE_INFO("Results: %d passed, %d failed, %d skipped.", passed, failed, skipped);
} 