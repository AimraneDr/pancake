#pragma once

#include "defines.h"

typedef struct Clock{
    f64 start_time;
    f64 elapsed;
}Clock;

//Updates The provided clock, should be called just before checking elapsed time.
//has no effect on non-started clocks.
PANCAKE_API void clock_update(Clock* clock);

//Starts the provided clock. reset the elapsed time
PANCAKE_API void clock_start(Clock* clock);

//Stops the provided clock, does not reset the elapsed time.
PANCAKE_API void clock_stop(Clock* clock);