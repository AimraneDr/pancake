#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "core/pancake_memory.h"
#include "game_types.h"

//Externally-defined function to create a game .
extern b8 create_game(game* out_game);

/*
    The Main Entry Point of The Application
*/
int main(void) {
    //request the game instance from the application
    game game_inst;
    if(!create_game(&game_inst)){
        PANCAKE_FATAL("Could not create the game !");
        return -1;
    }

    //Ensure the function pointers exists
    if(!game_inst.Initialize || !game_inst.Redner || !game_inst.state || !game_inst.Update){
        PANCAKE_FATAL("The Game's Function Pointers Must be assigned !");
        return -2;
    }

    //Initializing
    if(!application_create(&game_inst)){
        PANCAKE_FATAL("Application Failed to Create");
        return 1;
    }

    //Begin the Game Loop
    if(!application_run()){
         PANCAKE_FATAL("Application did not shutdown greacfully !");
        return 2;
    }
    return 0;
}