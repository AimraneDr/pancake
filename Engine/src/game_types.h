#pragma once

#include "core/application.h"

/*
    represent the basic game state in a game.
    Called for creation by the application
*/
typedef struct game{
    ApplicationConfig config;                                       // the application configuration .
    b8 (*Initialize)(struct game* game_inst);                       //function pointer to game's initialize function
    b8 (*Update)(struct game* game_inst, f32 delta_time);           //function pointer to game's update function
    b8 (*Redner)(struct game* game_inst, f32 delta_time);           //function pointer to game's update function
    void (*OnResize)(struct game* game_inst, u32 width, u32 height);//function pointer to handle resizes, if applicable
    void* state;                                                    //Game-specific game state .Created and managed by the game .
    void* application_state;                                        //holds the Application State
}game;