#include "pancake_math.h"
#include "platform/platform.h"

#include <math.h>
#include <stdlib.h>

static b8 rand_seeded = FALSE;

/**
 * Note that these are here in order to prevent having to import the
 * entire <math.h> everywhere.
 */
f32 pancake_sin(f32 x) {
    return sinf(x);
}

f32 pancake_cos(f32 x) {
    return cosf(x);
}

f32 pancake_tan(f32 x) {
    return tanf(x);
}

f32 pancake_acos(f32 x) {
    return acosf(x);
}

f32 pancake_sqrt(f32 x) {
    return sqrtf(x);
}

f32 pancake_abs(f32 x) {
    return fabsf(x);
}

i32 pancake_random() {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time());
        rand_seeded = TRUE;
    }
    return rand();
}

i32 pancake_random_in_range(i32 min, i32 max) {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time());
        rand_seeded = TRUE;
    }
    return (rand() % (max - min + 1)) + min;
}

f32 f_pancake_random() {
    return (float)pancake_random() / (f32)RAND_MAX;
}

f32 f_pancake_random_in_range(f32 min, f32 max) {
    return min + ((float)pancake_random() / ((f32)RAND_MAX / (max - min)));
}