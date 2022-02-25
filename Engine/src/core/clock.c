#include "clock.h"
#include "platform/platform.h"



void clock_update(Clock* clock){
    if(clock->start_time != 0){
        clock->elapsed = platform_get_absolute_time() - clock->start_time;
    }
}

void clock_start(Clock* clock){
    clock->start_time = platform_get_absolute_time();
    clock->elapsed = 0;
}

void clock_stop(Clock* clock){
    clock->start_time = 0;
}