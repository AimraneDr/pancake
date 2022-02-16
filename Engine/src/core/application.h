#pragma once

#include "defines.h"

struct game;

//Application Configuration
typedef struct ApplicationConfig{
    //window starting position x axis if applicable
    i16 start_pos_x;

    //window starting position y axis if applicable
    i16 start_pos_y;

    //window starting width if applicable
    i16 start_width;

    //window starting height if applicable
    i16 start_height;

    //application name used in windowing if applicable
    char* name;
}ApplicationConfig;


PANCAKE_API b8 application_create(struct game* game_inst);
PANCAKE_API b8 application_run();
