#include <entry.h>
#include <core/pancake_memory.h>
#include "game.h"


b8 create_game(game* out_game){
    //Application Configuration
    out_game->config.start_pos_x = 100;
    out_game->config.start_pos_y = 100;
    out_game->config.start_width = 600;
    out_game->config.start_height = 500;
    out_game->config.name = "Pancake Engine Testbed";
    out_game->Initialize = game_initialize;
    out_game->Update = game_update;
    out_game->Redner = game_render;
    out_game->OnResize = game_on_resize;

    //create the game state
    out_game->state = pancake_allocate(sizeof(game_state), MEMORY_TAG_GAME);
    
    out_game->application_state = 0;

    return true;
}